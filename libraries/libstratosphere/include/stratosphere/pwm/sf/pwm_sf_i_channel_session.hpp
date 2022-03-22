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

#pragma once
#include <vapours.hpp>
#include <stratosphere/pwm/pwm_types.hpp>

#define AMS_PWM_I_CHANNEL_SESSION_INTERFACE_INFO(C, H)                                                                \
    AMS_SF_METHOD_INFO(C, H,  0, Result, SetPeriod,  (TimeSpanType period),            (period)                     ) \
    AMS_SF_METHOD_INFO(C, H,  1, Result, GetPeriod,  (ams::sf::Out<TimeSpanType> out), (out)                        ) \
    AMS_SF_METHOD_INFO(C, H,  2, Result, SetDuty,    (int duty),                       (duty)                       ) \
    AMS_SF_METHOD_INFO(C, H,  3, Result, GetDuty,    (ams::sf::Out<int> out),          (out)                        ) \
    AMS_SF_METHOD_INFO(C, H,  4, Result, SetEnabled, (bool enabled),                   (enabled)                    ) \
    AMS_SF_METHOD_INFO(C, H,  5, Result, GetEnabled, (ams::sf::Out<bool> out),         (out)                        ) \
    AMS_SF_METHOD_INFO(C, H,  6, Result, SetScale,   (double scale),                   (scale),   hos::Version_6_0_0) \
    AMS_SF_METHOD_INFO(C, H,  7, Result, GetScale,   (ams::sf::Out<double> out),       (out),     hos::Version_6_0_0)

AMS_SF_DEFINE_INTERFACE(ams::pwm::sf, IChannelSession, AMS_PWM_I_CHANNEL_SESSION_INTERFACE_INFO)
