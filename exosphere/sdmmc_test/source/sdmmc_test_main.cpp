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

namespace ams::sdmmc_test {

    namespace {

        constexpr inline const uintptr_t PMC = secmon::MemoryRegionPhysicalDevicePmc.GetAddress();

        constexpr inline auto Port = sdmmc::Port_Mmc0;
        alignas(8) constinit u8 g_mmc_work_buffer[sdmmc::MmcWorkBufferSize];

        constexpr inline u32 SectorIndex = 0;
        constexpr inline u32 SectorCount = 2;

        NORETURN void PmcMainReboot() {
            /* Write enable to MAIN_RESET. */
            reg::Write(PMC + APBDEV_PMC_CNTRL, PMC_REG_BITS_ENUM(CNTRL_MAIN_RESET, ENABLE));

            /* Wait forever until we're reset. */
            AMS_INFINITE_LOOP();
        }

        void CheckResult(const Result result) {
            volatile u32 * const DEBUG = reinterpret_cast<volatile u32 *>(0x4003C000);
            if (R_FAILED(result)) {
                DEBUG[1] = result.GetValue();
                PmcMainReboot();
            }
        }

    }

    void Main() {
        /* Debug signaler. */
        volatile u32 * const DEBUG = reinterpret_cast<volatile u32 *>(0x4003C000);
        DEBUG[0] = 0;
        DEBUG[1] = 0xAAAAAAAA;

        /* Initialize sdmmc library. */
        sdmmc::Initialize(Port);
        DEBUG[0] = 1;

        sdmmc::SetMmcWorkBuffer(Port, g_mmc_work_buffer, sizeof(g_mmc_work_buffer));
        DEBUG[0] = 2;

        Result result = sdmmc::Activate(Port);
        DEBUG[0] = 3;
        CheckResult(result);

        /* Select user data partition. */
        result = sdmmc::SelectMmcPartition(Port, sdmmc::MmcPartition_UserData);
        DEBUG[0] = 4;
        CheckResult(result);

        /* Read the first two sectors from disk. */
        void * const sector_dst = reinterpret_cast<void *>(0x40038000);
        result = sdmmc::Read(sector_dst, SectorCount * sdmmc::SectorSize, Port, SectorIndex, SectorCount);
        DEBUG[0] = 5;
        CheckResult(result);

        /* Perform a reboot. */
        DEBUG[1] = 0;
        PmcMainReboot();
    }

    NORETURN void ExceptionHandler() {
        PmcMainReboot();
    }

}

namespace ams::diag {

    void AbortImpl() {
        sdmmc_test::ExceptionHandler();
    }

}
