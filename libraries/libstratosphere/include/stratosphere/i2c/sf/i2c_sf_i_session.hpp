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
#include <stratosphere/i2c/i2c_types.hpp>

namespace ams::i2c::sf {

    #define AMS_I2C_I_SESSION_INTERFACE_INFO(C, H)                                                                                                                                                                       \
        AMS_SF_METHOD_INFO(C, H,  0, Result, SendOld,               (const ams::sf::InBuffer &in_data, i2c::TransactionOption option),                                           hos::Version_Min,   hos::Version_5_1_0) \
        AMS_SF_METHOD_INFO(C, H,  1, Result, ReceiveOld,            (const ams::sf::OutBuffer &out_data, i2c::TransactionOption option),                                         hos::Version_Min,   hos::Version_5_1_0) \
        AMS_SF_METHOD_INFO(C, H,  2, Result, ExecuteCommandListOld, (const ams::sf::OutBuffer &rcv_buf, const ams::sf::InPointerArray<i2c::I2cCommand> &command_list),           hos::Version_Min,   hos::Version_5_1_0) \
        AMS_SF_METHOD_INFO(C, H, 10, Result, Send,                  (const ams::sf::InAutoSelectBuffer &in_data, i2c::TransactionOption option)                                                                        ) \
        AMS_SF_METHOD_INFO(C, H, 11, Result, Receive,               (const ams::sf::OutAutoSelectBuffer &out_data, i2c::TransactionOption option)                                                                      ) \
        AMS_SF_METHOD_INFO(C, H, 12, Result, ExecuteCommandList,    (const ams::sf::OutAutoSelectBuffer &rcv_buf, const ams::sf::InPointerArray<i2c::I2cCommand> &command_list)                                        ) \
        AMS_SF_METHOD_INFO(C, H, 13, Result, SetRetryPolicy,        (s32 max_retry_count, s32 retry_interval_us),                                                                hos::Version_6_0_0                    )

    AMS_SF_DEFINE_INTERFACE(ISession, AMS_I2C_I_SESSION_INTERFACE_INFO)

}
