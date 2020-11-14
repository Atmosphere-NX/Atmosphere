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
#include <stratosphere/pwm/pwm_select_channel_name.hpp>
#include <stratosphere/pwm/sf/pwm_sf_i_channel_session.hpp>

namespace ams::pwm::sf {

    #define AMS_PWM_I_MANAGER_INTERFACE_INFO(C, H)                                                                                                                                                \
        AMS_SF_METHOD_INFO(C, H,  0, Result, OpenSessionForDev,                  (ams::sf::Out<std::shared_ptr<pwm::sf::IChannelSession>> out, int channel)                                     ) \
        AMS_SF_METHOD_INFO(C, H,  1, Result, OpenSession,                        (ams::sf::Out<std::shared_ptr<pwm::sf::IChannelSession>> out, pwm::ChannelName channel_name)                   ) \
        AMS_SF_METHOD_INFO(C, H,  2, Result, OpenSession2,                       (ams::sf::Out<std::shared_ptr<pwm::sf::IChannelSession>> out, DeviceCode device_code),       hos::Version_6_0_0)

    AMS_SF_DEFINE_INTERFACE(IManager, AMS_PWM_I_MANAGER_INTERFACE_INFO)

}
