/*
 * Copyright (c) Atmosphère-NX
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
#include "fusee_sd_card.hpp"

namespace ams::nxboot {

    namespace {

        constexpr inline auto SdCardPort = sdmmc::Port_SdCard0;

        constexpr inline const uintptr_t APB = secmon::MemoryRegionPhysicalDeviceApbMisc.GetAddress();

        alignas(0x10) constinit u8 g_sd_work_buffer[sdmmc::SdCardWorkBufferSize];

        void ConfigureInitialSdCardPinmux() {
            /* Normally, these pints get configured by boot sysmodule during initial pinmux config. */
            /* However, they're required to access the SD card, so we must do them ahead of time. */
            reg::ReadWrite(APB + PINMUX_AUX_SDMMC1_CLK,  PINMUX_REG_BITS_ENUM(AUX_E_INPUT,             ENABLE),
                                                         PINMUX_REG_BITS_ENUM(AUX_TRISTATE,       PASSTHROUGH),
                                                         PINMUX_REG_BITS_ENUM(AUX_PUPD,             PULL_DOWN),
                                                         PINMUX_REG_BITS_ENUM(AUX_SDMMC1_CLK_PM,       SDMMC1));

            reg::ReadWrite(APB + PINMUX_AUX_SDMMC1_CMD,  PINMUX_REG_BITS_ENUM(AUX_E_INPUT,             ENABLE),
                                                         PINMUX_REG_BITS_ENUM(AUX_TRISTATE,       PASSTHROUGH),
                                                         PINMUX_REG_BITS_ENUM(AUX_PUPD,               PULL_UP),
                                                         PINMUX_REG_BITS_ENUM(AUX_SDMMC1_CMD_PM,       SDMMC1));

            reg::ReadWrite(APB + PINMUX_AUX_SDMMC1_DAT3, PINMUX_REG_BITS_ENUM(AUX_E_INPUT,             ENABLE),
                                                         PINMUX_REG_BITS_ENUM(AUX_TRISTATE,       PASSTHROUGH),
                                                         PINMUX_REG_BITS_ENUM(AUX_PUPD,               PULL_UP),
                                                         PINMUX_REG_BITS_ENUM(AUX_SDMMC1_DAT3_PM,      SDMMC1));

            reg::ReadWrite(APB + PINMUX_AUX_SDMMC1_DAT2, PINMUX_REG_BITS_ENUM(AUX_E_INPUT,             ENABLE),
                                                         PINMUX_REG_BITS_ENUM(AUX_TRISTATE,       PASSTHROUGH),
                                                         PINMUX_REG_BITS_ENUM(AUX_PUPD,               PULL_UP),
                                                         PINMUX_REG_BITS_ENUM(AUX_SDMMC1_DAT2_PM,      SDMMC1));

            reg::ReadWrite(APB + PINMUX_AUX_SDMMC1_DAT1, PINMUX_REG_BITS_ENUM(AUX_E_INPUT,             ENABLE),
                                                         PINMUX_REG_BITS_ENUM(AUX_TRISTATE,       PASSTHROUGH),
                                                         PINMUX_REG_BITS_ENUM(AUX_PUPD,               PULL_UP),
                                                         PINMUX_REG_BITS_ENUM(AUX_SDMMC1_DAT1_PM,      SDMMC1));

            reg::ReadWrite(APB + PINMUX_AUX_SDMMC1_DAT0, PINMUX_REG_BITS_ENUM(AUX_E_INPUT,             ENABLE),
                                                         PINMUX_REG_BITS_ENUM(AUX_TRISTATE,       PASSTHROUGH),
                                                         PINMUX_REG_BITS_ENUM(AUX_PUPD,               PULL_UP),
                                                         PINMUX_REG_BITS_ENUM(AUX_SDMMC1_DAT0_PM,      SDMMC1));

            reg::ReadWrite(APB + PINMUX_AUX_DMIC3_CLK,   PINMUX_REG_BITS_ENUM(AUX_E_OD,               DISABLE),
                                                         PINMUX_REG_BITS_ENUM(AUX_E_INPUT,            DISABLE),
                                                         PINMUX_REG_BITS_ENUM(AUX_TRISTATE,       PASSTHROUGH),
                                                         PINMUX_REG_BITS_ENUM(AUX_SDMMC1_DAT0_PM,       RSVD2));
        }

    }

    Result InitializeSdCard() {
        /* Perform initial pinmux config to enable sd card access. */
        ConfigureInitialSdCardPinmux();

        /* Initialize the SD card. */
        sdmmc::Initialize(SdCardPort);

        /* Set the SD card work buffer. */
        sdmmc::SetSdCardWorkBuffer(SdCardPort, g_sd_work_buffer, sizeof(g_sd_work_buffer));

        /* Activate the SD card. */
        return sdmmc::Activate(SdCardPort);
    }

    void FinalizeSdCard() {
        /* Deactivate the SD card. */
        sdmmc::Deactivate(SdCardPort);

        /* Finalize the SD card. */
        sdmmc::Finalize(SdCardPort);
    }

    Result CheckSdCardConnection(sdmmc::SpeedMode *out_sm, sdmmc::BusWidth *out_bw) {
        return sdmmc::CheckSdCardConnection(out_sm, out_bw, SdCardPort);
    }

    Result GetSdCardMemoryCapacity(u32 *out_num_sectors) {
        return sdmmc::GetDeviceMemoryCapacity(out_num_sectors, SdCardPort);
    }

    Result ReadSdCard(void *dst, size_t size, size_t sector_index, size_t sector_count) {
        return sdmmc::Read(dst, size, SdCardPort, sector_index, sector_count);
    }

    Result WriteSdCard(size_t sector_index, size_t sector_count, const void *src, size_t size) {
        return sdmmc::Write(SdCardPort, sector_index, sector_count, src, size);
    }

}