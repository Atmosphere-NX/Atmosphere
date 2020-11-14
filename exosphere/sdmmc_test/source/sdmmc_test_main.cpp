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

        constexpr inline auto Port = sdmmc::Port_SdCard0;
        alignas(8) constinit u8 g_sd_work_buffer[sdmmc::SdCardWorkBufferSize];

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
        /* Perform butchered hwinit. */
        /* TODO: replace with simpler, non-C logic. */
        /* nx_hwinit(); */

        /* Clear output buffer for debug. */
        std::memset((void *)0x40038000, 0xAA, 0x400);

        /* Normally, these pins get configured by boot sysmodule during initial pinmux config. */
        /* However, they're required to access the SD card. */
        {
            const uintptr_t apb_misc = dd::QueryIoMapping(0x70000000, 0x4000);

            reg::ReadWrite(apb_misc + PINMUX_AUX_SDMMC1_CLK,  PINMUX_REG_BITS_ENUM(AUX_E_INPUT,             ENABLE),
                                                              PINMUX_REG_BITS_ENUM(AUX_TRISTATE,       PASSTHROUGH),
                                                              PINMUX_REG_BITS_ENUM(AUX_PUPD,             PULL_DOWN),
                                                              PINMUX_REG_BITS_ENUM(AUX_SDMMC1_CLK_PM,       SDMMC1));

            reg::ReadWrite(apb_misc + PINMUX_AUX_SDMMC1_CMD,  PINMUX_REG_BITS_ENUM(AUX_E_INPUT,             ENABLE),
                                                              PINMUX_REG_BITS_ENUM(AUX_TRISTATE,       PASSTHROUGH),
                                                              PINMUX_REG_BITS_ENUM(AUX_PUPD,               PULL_UP),
                                                              PINMUX_REG_BITS_ENUM(AUX_SDMMC1_CMD_PM,       SDMMC1));

            reg::ReadWrite(apb_misc + PINMUX_AUX_SDMMC1_DAT3, PINMUX_REG_BITS_ENUM(AUX_E_INPUT,             ENABLE),
                                                              PINMUX_REG_BITS_ENUM(AUX_TRISTATE,       PASSTHROUGH),
                                                              PINMUX_REG_BITS_ENUM(AUX_PUPD,               PULL_UP),
                                                              PINMUX_REG_BITS_ENUM(AUX_SDMMC1_DAT3_PM,      SDMMC1));

            reg::ReadWrite(apb_misc + PINMUX_AUX_SDMMC1_DAT2, PINMUX_REG_BITS_ENUM(AUX_E_INPUT,             ENABLE),
                                                              PINMUX_REG_BITS_ENUM(AUX_TRISTATE,       PASSTHROUGH),
                                                              PINMUX_REG_BITS_ENUM(AUX_PUPD,               PULL_UP),
                                                              PINMUX_REG_BITS_ENUM(AUX_SDMMC1_DAT2_PM,      SDMMC1));

            reg::ReadWrite(apb_misc + PINMUX_AUX_SDMMC1_DAT1, PINMUX_REG_BITS_ENUM(AUX_E_INPUT,             ENABLE),
                                                              PINMUX_REG_BITS_ENUM(AUX_TRISTATE,       PASSTHROUGH),
                                                              PINMUX_REG_BITS_ENUM(AUX_PUPD,               PULL_UP),
                                                              PINMUX_REG_BITS_ENUM(AUX_SDMMC1_DAT1_PM,      SDMMC1));

            reg::ReadWrite(apb_misc + PINMUX_AUX_SDMMC1_DAT0, PINMUX_REG_BITS_ENUM(AUX_E_INPUT,             ENABLE),
                                                              PINMUX_REG_BITS_ENUM(AUX_TRISTATE,       PASSTHROUGH),
                                                              PINMUX_REG_BITS_ENUM(AUX_PUPD,               PULL_UP),
                                                              PINMUX_REG_BITS_ENUM(AUX_SDMMC1_DAT0_PM,      SDMMC1));

            reg::ReadWrite(apb_misc + PINMUX_AUX_DMIC3_CLK,   PINMUX_REG_BITS_ENUM(AUX_E_OD,               DISABLE),
                                                              PINMUX_REG_BITS_ENUM(AUX_E_INPUT,            DISABLE),
                                                              PINMUX_REG_BITS_ENUM(AUX_TRISTATE,       PASSTHROUGH),
                                                              PINMUX_REG_BITS_ENUM(AUX_SDMMC1_DAT0_PM,       RSVD2));
        }

        /* Debug signaler. */
        volatile u32 * const DEBUG = reinterpret_cast<volatile u32 *>(0x4003C000);
        DEBUG[0] = 0;
        DEBUG[1] = 0xAAAAAAAA;

        /* Initialize sdmmc library. */
        sdmmc::Initialize(Port);
        DEBUG[0] = 1;

        sdmmc::SetSdCardWorkBuffer(Port, g_sd_work_buffer, sizeof(g_sd_work_buffer));
        DEBUG[0] = 2;

        Result result = sdmmc::Activate(Port);
        DEBUG[0] = 3;
        CheckResult(result);

        /* Read the first two sectors from disk. */
        void * const sector_dst = reinterpret_cast<void *>(0x40038000);
        result = sdmmc::Read(sector_dst, SectorCount * sdmmc::SectorSize, Port, SectorIndex, SectorCount);
        DEBUG[0] = 4;
        CheckResult(result);

        /* Get the connection status. */
        sdmmc::SpeedMode speed_mode;
        sdmmc::BusWidth bus_width;
        result = sdmmc::CheckSdCardConnection(std::addressof(speed_mode), std::addressof(bus_width), Port);

        /* Save status for debug. */
        DEBUG[0] = 5;
        DEBUG[1] = result.GetValue();
        DEBUG[2] = static_cast<u32>(speed_mode);
        DEBUG[3] = static_cast<u32>(bus_width);

        /* Perform a reboot. */
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
