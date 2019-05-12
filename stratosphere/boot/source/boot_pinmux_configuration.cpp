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
#include "boot_pinmux_map.hpp"
#include "boot_pinmux_initial_configuration_icosa.hpp"
#include "boot_pinmux_initial_configuration_copper.hpp"
#include "boot_pinmux_initial_configuration_hoag.hpp"
#include "boot_pinmux_initial_configuration_iowa.hpp"
#include "boot_pinmux_initial_drive_pad_configuration.hpp"

void Boot::ConfigurePinmux() {
    /* Update parks. */
    for (size_t i = 0; i < PinmuxPadNameMax; i++) {
        Boot::PinmuxUpdatePark(static_cast<u32>(i));
    }

    /* Dummy read all drive pads. */
    for (size_t i = 0; i < PinmuxDrivePadNameMax; i++) {
        Boot::PinmuxDummyReadDrivePad(static_cast<u32>(i));
    }

    /* Set initial pad configs. */
    Boot::ConfigurePinmuxInitialPads();

    /* Set initial drive pad configs. */
    Boot::ConfigurePinmuxInitialDrivePads();
}

void Boot::ConfigurePinmuxInitialPads() {
    const PinmuxInitialConfig *configs = nullptr;
    size_t num_configs = 0;
    const HardwareType hw_type = Boot::GetHardwareType();

    switch (hw_type) {
        case HardwareType_Icosa:
            configs = PinmuxInitialConfigsIcosa;
            num_configs = PinmuxNumInitialConfigsIcosa;
            break;
        case HardwareType_Copper:
            configs = PinmuxInitialConfigsCopper;
            num_configs = PinmuxNumInitialConfigsCopper;
            break;
        case HardwareType_Hoag:
            configs = PinmuxInitialConfigsHoag;
            num_configs = PinmuxNumInitialConfigsHoag;
            break;
        case HardwareType_Iowa:
            configs = PinmuxInitialConfigsIowa;
            num_configs = PinmuxNumInitialConfigsIowa;
            break;
        default:
            /* Unknown hardware type, we can't proceed. */
            std::abort();
    }

    /* Ensure we found an appropriate config. */
    if (configs == nullptr) {
        std::abort();
    }

    for (size_t i = 0; i < num_configs - 1; i++) {
        Boot::PinmuxUpdatePad(configs[i].name, configs[i].val, configs[i].mask);
    }

    /* Extra configs for iowa only. */
    if (hw_type == HardwareType_Iowa) {
        static constexpr u32 ExtraIowaPinmuxPadNames[] = {
            0xAA, 0xAC, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9
        };
        for (size_t i = 0; i < sizeof(ExtraIowaPinmuxPadNames) / sizeof(ExtraIowaPinmuxPadNames[0]); i++) {
            Boot::PinmuxUpdatePad(ExtraIowaPinmuxPadNames[i], 0x2000, 0x2000);
        }
    }
}

void Boot::ConfigurePinmuxInitialDrivePads() {
    const PinmuxInitialConfig *configs = PinmuxInitialDrivePadConfigs;
    for (size_t i = 0; i < PinmuxNumInitialDrivePadConfigs; i++) {
        Boot::PinmuxUpdateDrivePad(configs[i].name, configs[i].val, configs[i].mask);
    }
}
