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

#pragma once
#include <vapours.hpp>
#include <stratosphere/ddsf/ddsf_types.hpp>
#include <stratosphere/gpio/gpio_types.hpp>
#include <stratosphere/gpio/gpio_select_pad_name.hpp>
#include <stratosphere/gpio/sf/gpio_sf_i_pad_session.hpp>

namespace ams::gpio::sf {

    #define AMS_GPIO_I_MANAGER_INTERFACE_INFO(C, H)                                                                                                                                                                                          \
        AMS_SF_METHOD_INFO(C, H,  0, Result, OpenSessionForDev,                  (ams::sf::Out<std::shared_ptr<gpio::sf::IPadSession>> out, s32 pad_descriptor)                                                                          )   \
        AMS_SF_METHOD_INFO(C, H,  1, Result, OpenSession,                        (ams::sf::Out<std::shared_ptr<gpio::sf::IPadSession>> out, gpio::GpioPadName pad_name)                                                                  )   \
        AMS_SF_METHOD_INFO(C, H,  2, Result, OpenSessionForTest,                 (ams::sf::Out<std::shared_ptr<gpio::sf::IPadSession>> out, gpio::GpioPadName pad_name)                                                                  )   \
        AMS_SF_METHOD_INFO(C, H,  3, Result, IsWakeEventActive,                  (ams::sf::Out<bool> out, gpio::GpioPadName pad_name),                                                             hos::Version_Min,   hos::Version_6_2_0)   \
        AMS_SF_METHOD_INFO(C, H,  4, Result, GetWakeEventActiveFlagSet,          (ams::sf::Out<gpio::WakeBitFlag> out),                                                                            hos::Version_Min,   hos::Version_6_2_0)   \
        AMS_SF_METHOD_INFO(C, H,  5, Result, SetWakeEventActiveFlagSetForDebug,  (gpio::GpioPadName pad_name, bool is_enabled),                                                                    hos::Version_Min,   hos::Version_6_2_0)   \
        AMS_SF_METHOD_INFO(C, H,  6, Result, SetWakePinDebugMode,                (s32 mode)                                                                                                                                              )   \
        AMS_SF_METHOD_INFO(C, H,  7, Result, OpenSession2,                       (ams::sf::Out<std::shared_ptr<gpio::sf::IPadSession>> out, DeviceCode device_code, ddsf::AccessMode access_mode), hos::Version_5_0_0                    )   \
        AMS_SF_METHOD_INFO(C, H,  8, Result, IsWakeEventActive2,                 (ams::sf::Out<bool> out, DeviceCode device_code),                                                                 hos::Version_5_0_0                    )   \
        AMS_SF_METHOD_INFO(C, H,  9, Result, SetWakeEventActiveFlagSetForDebug2, (DeviceCode device_code, bool is_enabled),                                                                        hos::Version_5_0_0                    )   \
        AMS_SF_METHOD_INFO(C, H, 10, Result, SetRetryValues,                     (u32 arg0, u32 arg1),                                                                                             hos::Version_6_0_0                    )

    AMS_SF_DEFINE_INTERFACE(IManager, AMS_GPIO_I_MANAGER_INTERFACE_INFO)

}
