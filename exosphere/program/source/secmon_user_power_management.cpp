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
#include "secmon_map.hpp"
#include "secmon_page_mapper.hpp"
#include "secmon_user_power_management.hpp"

#include "rebootstub_bin.h"

namespace ams::secmon {

    namespace {

        constexpr inline const uintptr_t PMC = MemoryRegionVirtualDevicePmc.GetAddress();

        constexpr inline const u32 RebootStubPhysicalAddress = MemoryRegionPhysicalIramRebootStub.GetAddress();

        enum RebootStubAction {
            RebootStubAction_ShutDown      = 0,
            RebootStubAction_JumpToPayload = 1,
        };

        NORETURN void PerformPmcReboot() {
            /* Write MAIN_RST. */
            reg::Write(PMC + APBDEV_PMC_CNTRL, 0x10);

            while (true) {
                /* ... */
            }
        }

        void LoadRebootStub(u32 action) {
            /* Configure the bootrom to boot to warmboot payload. */
            reg::Write(PMC + APBDEV_PMC_SCRATCH0, 0x1);

            /* Patch the bootrom to perform an SVC immediately after the second spare write. */
            reg::Write(PMC + APBDEV_PMC_SCRATCH45, 0x2E38DFFF);
            reg::Write(PMC + APBDEV_PMC_SCRATCH46, 0x6001DC28);

            /* Patch the bootrom to jump to the reboot stub we'll prepare in iram on SVC. */
            reg::Write(PMC + APBDEV_PMC_SCRATCH33, RebootStubPhysicalAddress);
            reg::Write(PMC + APBDEV_PMC_SCRATCH40, 0x6000F208);

            {
                /* Map the iram page. */
                AtmosphereIramPageMapper mapper(RebootStubPhysicalAddress);
                AMS_ABORT_UNLESS(mapper.Map());

                /* Copy the reboot stub. */
                AMS_ABORT_UNLESS(mapper.CopyToMapping(RebootStubPhysicalAddress, rebootstub_bin, rebootstub_bin_size));

                /* Set the reboot type. */
                AMS_ABORT_UNLESS(mapper.CopyToMapping(RebootStubPhysicalAddress + 4, std::addressof(action), sizeof(action)));
            }
        }

    }

    void PerformUserRebootToRcm() {
        /* Configure the bootrom to boot to rcm. */
        reg::Write(PMC + APBDEV_PMC_SCRATCH0, 0x2);

        /* Reboot. */
        PerformPmcReboot();
    }

    void PerformUserRebootToPayload() {
        /* Load our reboot stub to iram. */
        LoadRebootStub(RebootStubAction_JumpToPayload);

        /* Reboot. */
        PerformPmcReboot();
    }

    void PerformUserRebootToFatalError() {
        if (fuse::GetSocType() == fuse::SocType_Erista) {
            /* On Erista, we reboot to fatal error by jumping to fusee primary's handler. */
            return PerformUserRebootToPayload();
        } else /* if (fuse::GetSocType() == fuse::SocType_Mariko) */ {
            /* TODO: Send a SGI FIQ to the other CPUs, so that user code stops executing. */

            /* TODO: On cores other than 3, halt/wfi. */

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
        }
    }

    void PerformUserShutDown() {
        /* Load our reboot stub to iram. */
        LoadRebootStub(RebootStubAction_ShutDown);

        /* Reboot. */
        PerformPmcReboot();
    }

}
