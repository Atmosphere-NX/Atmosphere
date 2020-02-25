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
#include "pinmux_initial_configuration.hpp"
#include "pinmux_utils.hpp"

namespace ams::pinmux {

    namespace {

        struct InitialConfig {
            u32 name;
            u32 val;
            u32 mask;
        };

        /* Include all initial configuration definitions. */
#include "pinmux_initial_configuration_icosa.inc"
#include "pinmux_initial_configuration_copper.inc"
#include "pinmux_initial_configuration_hoag.inc"
#include "pinmux_initial_configuration_iowa.inc"
#include "pinmux_initial_configuration_calcio.inc"

#include "pinmux_initial_drive_pad_configuration.inc"
#include "pinmux_initial_drive_pad_configuration_hoag.inc"


        /* Configuration helpers. */
        void ConfigureInitialPads() {
            const InitialConfig *configs = nullptr;
            size_t num_configs = 0;
            const auto hw_type = spl::GetHardwareType();

            switch (hw_type) {
                case spl::HardwareType::Icosa:
                    configs     = InitialConfigsIcosa;
                    num_configs = NumInitialConfigsIcosa;
                    break;
                case spl::HardwareType::Copper:
                    configs     = InitialConfigsCopper;
                    num_configs = NumInitialConfigsCopper;
                    break;
                case spl::HardwareType::Hoag:
                    configs     = InitialConfigsHoag;
                    num_configs = NumInitialConfigsHoag;
                    break;
                case spl::HardwareType::Iowa:
                    configs     = InitialConfigsIowa;
                    num_configs = NumInitialConfigsIowa;
                    break;
                case spl::HardwareType::Calcio:
                    configs     = InitialConfigsCalcio;
                    num_configs = NumInitialConfigsCalcio;
                    break;
                /* Unknown hardware type, we can't proceed. */
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            /* Ensure we found an appropriate config. */
            AMS_ABORT_UNLESS(configs != nullptr);

            for (size_t i = 0; i < num_configs; i++) {
                UpdatePad(configs[i].name, configs[i].val, configs[i].mask);
            }

            /* Extra configs for mariko only. */
            if (hw_type == spl::HardwareType::Hoag || hw_type == spl::HardwareType::Iowa || hw_type == spl::HardwareType::Calcio) {
                static constexpr u32 ExtraMarikoPadNames[] = {
                    0xAA, 0xAC, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9
                };
                for (size_t i = 0; i < util::size(ExtraMarikoPadNames); i++) {
                    UpdatePad(ExtraMarikoPadNames[i], 0x2000, 0x2000);
                }
            }
        }

        void ConfigureInitialDrivePads() {
            const InitialConfig *configs = nullptr;
            size_t num_configs = 0;
            const auto hw_type = spl::GetHardwareType();

            switch (hw_type) {
                case spl::HardwareType::Icosa:
                case spl::HardwareType::Copper:
                case spl::HardwareType::Iowa:
                case spl::HardwareType::Calcio:
                    configs     = InitialDrivePadConfigs;
                    num_configs = NumInitialDrivePadConfigs;
                    break;
                case spl::HardwareType::Hoag:
                    configs     = InitialDrivePadConfigsHoag;
                    num_configs = NumInitialDrivePadConfigsHoag;
                    break;
                /* Unknown hardware type, we can't proceed. */
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            /* Ensure we found an appropriate config. */
            AMS_ABORT_UNLESS(configs != nullptr);

            for (size_t i = 0; i < num_configs; i++) {
                UpdateDrivePad(configs[i].name, configs[i].val, configs[i].mask);
            }
        }

    }

    void SetInitialConfiguration() {
        /* Update all parks. */
        UpdateAllParks();

        /* Dummy read all drive pads. */
        DummyReadAllDrivePads();

        /* Set initial pad configs. */
        ConfigureInitialPads();

        /* Set initial drive pad configs. */
        ConfigureInitialDrivePads();
    }

}
