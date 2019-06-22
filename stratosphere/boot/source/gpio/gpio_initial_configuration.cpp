/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include <stratosphere/spl.hpp>

#include "gpio_initial_configuration.hpp"
#include "gpio_utils.hpp"

namespace sts::gpio {

    namespace {

        struct InitialConfig {
            u32 pad_name;
            GpioDirection direction;
            GpioValue value;
        };

        /* Include all initial configuration definitions. */
#include "gpio_initial_configuration_icosa.inc"
#include "gpio_initial_configuration_copper.inc"
#include "gpio_initial_configuration_hoag.inc"
#include "gpio_initial_configuration_iowa.inc"

    }

    void SetInitialConfiguration() {
        const InitialConfig *configs = nullptr;
        size_t num_configs = 0;
        const auto hw_type = spl::GetHardwareType();
        const FirmwareVersion fw_ver = GetRuntimeFirmwareVersion();

        /* Choose GPIO map. */
        if (fw_ver >= FirmwareVersion_200) {
            switch (hw_type) {
                case spl::HardwareType::Icosa:
                    {
                        if (fw_ver >= FirmwareVersion_400) {
                            configs = InitialConfigsIcosa4x;
                            num_configs = NumInitialConfigsIcosa4x;
                        } else {
                            configs = InitialConfigsIcosa;
                            num_configs = NumInitialConfigsIcosa;
                        }
                    }
                    break;
                case spl::HardwareType::Copper:
                    configs = InitialConfigsCopper;
                    num_configs = NumInitialConfigsCopper;
                    break;
                case spl::HardwareType::Hoag:
                    configs = InitialConfigsHoag;
                    num_configs = NumInitialConfigsHoag;
                    break;
                case spl::HardwareType::Iowa:
                    configs = InitialConfigsIowa;
                    num_configs = NumInitialConfigsIowa;
                    break;
                default:
                    /* Unknown hardware type, we can't proceed. */
                    std::abort();
            }
        } else {
            /* Until 2.0.0, the GPIO map for Icosa was used for all hardware types. */
            configs = InitialConfigsIcosa;
            num_configs = NumInitialConfigsIcosa;
        }

        /* Ensure we found an appropriate config. */
        if (configs == nullptr) {
            std::abort();
        }

        for (size_t i = 0; i < num_configs; i++) {
            /* Configure the GPIO. */
            Configure(configs[i].pad_name);

            /* Set the GPIO's direction. */
            SetDirection(configs[i].pad_name, configs[i].direction);

            if (configs[i].direction == GpioDirection_Output) {
                /* Set the GPIO's value. */
                SetValue(configs[i].pad_name, configs[i].value);
            }
        }
    }

}

