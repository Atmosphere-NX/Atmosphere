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

#include "boot_functions.hpp"
#include "boot_gpio_initial_configuration_icosa.hpp"
#include "boot_gpio_initial_configuration_copper.hpp"
#include "boot_gpio_initial_configuration_hoag.hpp"
#include "boot_gpio_initial_configuration_iowa.hpp"

void Boot::SetInitialGpioConfiguration() {
    const GpioInitialConfig *configs = nullptr;
    size_t num_configs = 0;
    const HardwareType hw_type = Boot::GetHardwareType();
    const FirmwareVersion fw_ver = GetRuntimeFirmwareVersion();

    /* Choose GPIO map. */
    if (fw_ver >= FirmwareVersion_200) {
        switch (hw_type) {
            case HardwareType_Icosa:
                {
                    if (fw_ver >= FirmwareVersion_400) {
                        configs = GpioInitialConfigsIcosa4x;
                        num_configs = GpioNumInitialConfigsIcosa4x;
                    } else {
                        configs = GpioInitialConfigsIcosa;
                        num_configs = GpioNumInitialConfigsIcosa;
                    }
                }
                break;
            case HardwareType_Copper:
                configs = GpioInitialConfigsCopper;
                num_configs = GpioNumInitialConfigsCopper;
                break;
            case HardwareType_Hoag:
                configs = GpioInitialConfigsHoag;
                num_configs = GpioNumInitialConfigsHoag;
                break;
            case HardwareType_Iowa:
                configs = GpioInitialConfigsIowa;
                num_configs = GpioNumInitialConfigsIowa;
                break;
            default:
                /* Unknown hardware type, we can't proceed. */
                std::abort();
        }
    } else {
        /* Until 2.0.0, the GPIO map for Icosa was used for all hardware types. */
        configs = GpioInitialConfigsIcosa;
        num_configs = GpioNumInitialConfigsIcosa;
    }

    /* Ensure we found an appropriate config. */
    if (configs == nullptr) {
        std::abort();
    }

    for (size_t i = 0; i < num_configs; i++) {
        /* Configure the GPIO. */
        Boot::GpioConfigure(configs[i].pad_name);

        /* Set the GPIO's direction. */
        Boot::GpioSetDirection(configs[i].pad_name, configs[i].direction);

        if (configs[i].direction == GpioDirection_Output) {
            /* Set the GPIO's value. */
            Boot::GpioSetValue(configs[i].pad_name, configs[i].value);
        }
    }
}