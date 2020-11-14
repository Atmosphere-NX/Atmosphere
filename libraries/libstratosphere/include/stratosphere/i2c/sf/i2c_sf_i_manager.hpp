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
#include <stratosphere/i2c/i2c_select_device_name.hpp>
#include <stratosphere/i2c/sf/i2c_sf_i_session.hpp>

namespace ams::i2c::sf {

    #define AMS_I2C_I_MANAGER_INTERFACE_INFO(C, H)                                                                                                                                                                                                                              \
        AMS_SF_METHOD_INFO(C, H,  0, Result, OpenSessionForDev,                  (ams::sf::Out<std::shared_ptr<i2c::sf::ISession>> out, s32 bus_idx, u16 slave_address, i2c::AddressingMode addressing_mode, i2c::SpeedMode speed_mode)                                        ) \
        AMS_SF_METHOD_INFO(C, H,  1, Result, OpenSession,                        (ams::sf::Out<std::shared_ptr<i2c::sf::ISession>> out, i2c::I2cDevice device)                                                                                                                 ) \
        AMS_SF_METHOD_INFO(C, H,  2, Result, HasDevice,                          (ams::sf::Out<bool> out, i2c::I2cDevice device),                                                                                                        hos::Version_Min,   hos::Version_5_1_0) \
        AMS_SF_METHOD_INFO(C, H,  3, Result, HasDeviceForDev,                    (ams::sf::Out<bool> out, i2c::I2cDevice device),                                                                                                        hos::Version_Min,   hos::Version_5_1_0) \
        AMS_SF_METHOD_INFO(C, H,  4, Result, OpenSession2,                       (ams::sf::Out<std::shared_ptr<i2c::sf::ISession>> out, DeviceCode device_code),                                                                         hos::Version_6_0_0                    )

    AMS_SF_DEFINE_INTERFACE(IManager, AMS_I2C_I_MANAGER_INTERFACE_INFO)

}
