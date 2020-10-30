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
#include <stratosphere/gpio/gpio_types.hpp>

namespace ams::gpio::sf {

    #define AMS_GPIO_I_PAD_SESSION_INTERFACE_INFO(C, H)                                                                            \
        AMS_SF_METHOD_INFO(C, H,  0, Result, SetDirection,          (gpio::Direction direction)                                  ) \
        AMS_SF_METHOD_INFO(C, H,  1, Result, GetDirection,          (ams::sf::Out<gpio::Direction> out)                          ) \
        AMS_SF_METHOD_INFO(C, H,  2, Result, SetInterruptMode,      (gpio::InterruptMode mode)                                   ) \
        AMS_SF_METHOD_INFO(C, H,  3, Result, GetInterruptMode,      (ams::sf::Out<gpio::InterruptMode> out)                      ) \
        AMS_SF_METHOD_INFO(C, H,  4, Result, SetInterruptEnable,    (bool enable)                                                ) \
        AMS_SF_METHOD_INFO(C, H,  5, Result, GetInterruptEnable,    (ams::sf::Out<bool> out)                                     ) \
        AMS_SF_METHOD_INFO(C, H,  6, Result, GetInterruptStatus,    (ams::sf::Out<gpio::InterruptStatus> out)                    ) \
        AMS_SF_METHOD_INFO(C, H,  7, Result, ClearInterruptStatus,  ()                                                           ) \
        AMS_SF_METHOD_INFO(C, H,  8, Result, SetValue,              (gpio::GpioValue value)                                      ) \
        AMS_SF_METHOD_INFO(C, H,  9, Result, GetValue,              (ams::sf::Out<gpio::GpioValue> out)                          ) \
        AMS_SF_METHOD_INFO(C, H, 10, Result, BindInterrupt,         (ams::sf::OutCopyHandle out)                                 ) \
        AMS_SF_METHOD_INFO(C, H, 11, Result, UnbindInterrupt,       ()                                                           ) \
        AMS_SF_METHOD_INFO(C, H, 12, Result, SetDebounceEnabled,    (bool enable)                                                ) \
        AMS_SF_METHOD_INFO(C, H, 13, Result, GetDebounceEnabled,    (ams::sf::Out<bool> out)                                     ) \
        AMS_SF_METHOD_INFO(C, H, 14, Result, SetDebounceTime,       (s32 ms)                                                     ) \
        AMS_SF_METHOD_INFO(C, H, 15, Result, GetDebounceTime,       (ams::sf::Out<s32> out)                                      ) \
        AMS_SF_METHOD_INFO(C, H, 16, Result, SetValueForSleepState, (gpio::GpioValue value),                   hos::Version_4_0_0) \
        AMS_SF_METHOD_INFO(C, H, 16, Result, GetValueForSleepState, (ams::sf::Out<gpio::GpioValue> out),       hos::Version_6_0_0)

    AMS_SF_DEFINE_INTERFACE(IPadSession, AMS_GPIO_I_PAD_SESSION_INTERFACE_INFO)

}
