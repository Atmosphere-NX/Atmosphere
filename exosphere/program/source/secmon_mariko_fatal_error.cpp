/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <exosphere.hpp>
#include "secmon_cpu_context.hpp"
#include "secmon_map.hpp"
#include "secmon_page_mapper.hpp"
#include "secmon_mariko_fatal_error.hpp"
#include "smc/secmon_smc_power_management.hpp"

namespace ams::secmon {

    namespace {

        constinit u8 g_fatal_error_mask = 0;

    }

    void HandleMarikoFatalErrorInterrupt() {
        /* This interrupt handler doesn't return, so mark that we're at end of interrupt. */
        gic::SetEndOfInterrupt(MarikoFatalErrorInterruptId);

        /* Get the current core id. */
        const auto core_id = hw::GetCurrentCoreId();

        /* Set that we received the fatal on the current core. */
        g_fatal_error_mask |= (1u << core_id);
        hw::FlushDataCache(std::addressof(g_fatal_error_mask), sizeof(g_fatal_error_mask));
        hw::DataSynchronizationBarrier();

        /* If not all cores have received the fatal, we need to trigger the interrupt on other cores. */
        if (g_fatal_error_mask != (1u << NumCores) - 1) {
            /* Configure and send the interrupt to the next core. */
            const auto next_core = __builtin_ctz(~g_fatal_error_mask);
            gic::SetSpiTargetCpu(MarikoFatalErrorInterruptId, (1u << next_core));
            gic::SetPending(MarikoFatalErrorInterruptId);
        }

        /* If current core is not 3, kill ourselves. */
        if (core_id != NumCores - 1) {
            smc::PowerOffCpu();
        } else {
            /* Wait for all cores to kill themselves. */
            while (g_fatal_error_mask != (1u << NumCores) - 1) {
                util::WaitMicroSeconds(100);
            }
        }

        /* Copy the fatal error context to mariko tzram. */
        {
            /* Map the iram page. */
            constexpr uintptr_t FatalErrorPhysicalAddress = MemoryRegionPhysicalIramFatalErrorContext.GetAddress();
            AtmosphereIramPageMapper mapper(FatalErrorPhysicalAddress);
            if (mapper.Map()) {
                /* Copy the fatal error context. */
                      void *dst = MemoryRegionVirtualTzramMarikoProgramFatalErrorContext.GetPointer<void>();
                const void *src = mapper.GetPointerTo(FatalErrorPhysicalAddress, sizeof(ams::impl::FatalErrorContext));
                std::memcpy(dst, src, sizeof(ams::impl::FatalErrorContext));
            }
        }

        /* Map Dram for the mariko program. */
        MapDramForMarikoProgram();

        AMS_SECMON_LOG("%s\n", "Jumping to Mariko Fatal.");
        AMS_LOG_FLUSH();

        /* Jump to the mariko fatal program. */
        reinterpret_cast<void (*)()>(secmon::MemoryRegionVirtualTzramMarikoProgram.GetAddress())();

        /* The mariko fatal program never returns. */
        __builtin_unreachable();

        AMS_INFINITE_LOOP();
    }

}
