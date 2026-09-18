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
#include <stratosphere.hpp>
#include "pinmux_pad_index.hpp"
#include "pinmux_board_driver_api.hpp"
#include "pinmux_platform_pads.hpp"

namespace ams::pinmux::driver::board::nintendo::nx {

    namespace {

        constinit bool g_initialized = false;

        #include "pinmux_initial_pad_config_icosa.inc"
        #include "pinmux_initial_pad_config_hoag.inc"
        #include "pinmux_initial_pad_config_iowa.inc"
        #include "pinmux_initial_pad_config_calcio.inc"
        #include "pinmux_initial_pad_config_aula.inc"

        #include "pinmux_initial_drive_pad_config.inc"
        #include "pinmux_initial_drive_pad_config_hoag.inc"

    }

    bool IsInitialized() {
        return g_initialized;
    }

    void Initialize() {
        InitializePlatformPads();
        g_initialized = true;
    }

    void Finalize() {
        /* ... */
    }

    void SetInitialConfig() {
        const PinmuxPadConfig *configs = nullptr;
        size_t num_configs = 0;
        bool is_mariko = false;
        switch (spl::GetHardwareType()) {
            case spl::HardwareType::Icosa:
                configs     = PinmuxPadConfigsIcosa;
                num_configs = NumPinmuxPadConfigsIcosa;
                is_mariko = false;
                break;
            case spl::HardwareType::Hoag:
                configs     = PinmuxPadConfigsHoag;
                num_configs = NumPinmuxPadConfigsHoag;
                is_mariko = true;
                break;
            case spl::HardwareType::Iowa:
                configs     = PinmuxPadConfigsIowa;
                num_configs = NumPinmuxPadConfigsIowa;
                is_mariko = true;
                break;
            case spl::HardwareType::Calcio:
                configs     = PinmuxPadConfigsCalcio;
                num_configs = NumPinmuxPadConfigsCalcio;
                is_mariko = true;
                break;
            case spl::HardwareType::Aula:
                configs     = PinmuxPadConfigsAula;
                num_configs = NumPinmuxPadConfigsAula;
                is_mariko = true;
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        AMS_ABORT_UNLESS(configs != nullptr);

        for (size_t i = 0; i < num_configs; ++i) {
            UpdateSinglePinmuxPad(configs[i]);
        }

        if (is_mariko) {
            UpdateSinglePinmuxPad({ PinmuxPadIndex_Sdmmc2Clk,  0x2000, 0x2000 });
            UpdateSinglePinmuxPad({ PinmuxPadIndex_Sdmmc2Cmd,  0x2000, 0x2000 });
            UpdateSinglePinmuxPad({ PinmuxPadIndex_Sdmmc2Dat0, 0x2000, 0x2000 });
            UpdateSinglePinmuxPad({ PinmuxPadIndex_Sdmmc2Dat1, 0x2000, 0x2000 });
            UpdateSinglePinmuxPad({ PinmuxPadIndex_Sdmmc2Dat2, 0x2000, 0x2000 });
            UpdateSinglePinmuxPad({ PinmuxPadIndex_Sdmmc2Dat3, 0x2000, 0x2000 });
            UpdateSinglePinmuxPad({ PinmuxPadIndex_Sdmmc2Dat4, 0x2000, 0x2000 });
            UpdateSinglePinmuxPad({ PinmuxPadIndex_Sdmmc2Dat5, 0x2000, 0x2000 });
            UpdateSinglePinmuxPad({ PinmuxPadIndex_Sdmmc2Dat6, 0x2000, 0x2000 });
            UpdateSinglePinmuxPad({ PinmuxPadIndex_Sdmmc2Dat7, 0x2000, 0x2000 });
        }

        auto [log_port, log_baud_rate] = spl::GetLogConfiguration();
        AMS_UNUSED(log_baud_rate);

        if (log_port == 1) {
            UpdateSinglePinmuxPad({ 
                .index       = PinmuxPadIndex_Uart2Tx,
                .option      = (u32)PinmuxPadPm_Pm0 | PinmuxOpt_NoPupd | PinmuxOpt_Output,
                .option_mask = (u32)PinmuxOptBitMask_Pm | PinmuxOptBitMask_Pupd | PinmuxOptBitMask_Dir,
            });
            UpdateSinglePinmuxPad({ 
                .index       = PinmuxPadIndex_Uart2Cts,
                .option      = (u32)PinmuxPadPm_Pm0 | PinmuxOpt_NoPupd | PinmuxOpt_Input,
                .option_mask = (u32)PinmuxOptBitMask_Pm | PinmuxOptBitMask_Pupd | PinmuxOptBitMask_Dir,
            });
        }
        if (log_port == 2) {
            UpdateSinglePinmuxPad({ 
                .index       = PinmuxPadIndex_Uart3Tx,
                .option      = (u32)PinmuxPadPm_Pm0 | PinmuxOpt_NoPupd | PinmuxOpt_Output,
                .option_mask = (u32)PinmuxOptBitMask_Pm | PinmuxOptBitMask_Pupd | PinmuxOptBitMask_Dir,
            });
            UpdateSinglePinmuxPad({ 
                .index       = PinmuxPadIndex_Uart3Cts,
                .option      = (u32)PinmuxPadPm_Pm0 | PinmuxOpt_NoPupd | PinmuxOpt_Input,
                .option_mask = (u32)PinmuxOptBitMask_Pm | PinmuxOptBitMask_Pupd | PinmuxOptBitMask_Dir,
            });
        }
    }

    void SetInitialDrivePadConfig() {
        const PinmuxDrivePadConfig *configs = nullptr;
        size_t num_configs = 0;
        switch (spl::GetHardwareType()) {
            case spl::HardwareType::Icosa:
                configs     = PinmuxDrivePadConfigs;
                num_configs = NumPinmuxDrivePadConfigs;
                break;
            case spl::HardwareType::Hoag:
                configs     = PinmuxDrivePadConfigsHoag;
                num_configs = NumPinmuxDrivePadConfigsHoag;
                break;
            case spl::HardwareType::Iowa:
                configs     = PinmuxDrivePadConfigs;
                num_configs = NumPinmuxDrivePadConfigs;
                break;
            case spl::HardwareType::Calcio:
                configs     = PinmuxDrivePadConfigs;
                num_configs = NumPinmuxDrivePadConfigs;
                break;
            case spl::HardwareType::Aula:
                configs     = PinmuxDrivePadConfigs;
                num_configs = NumPinmuxDrivePadConfigs;
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        AMS_ABORT_UNLESS(configs != nullptr);

        for (size_t i = 0; i < num_configs; ++i) {
            UpdateSinglePinmuxDrivePad(configs[i]);
        }
    }

}
