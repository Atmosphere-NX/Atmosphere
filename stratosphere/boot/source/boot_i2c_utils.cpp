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
#include <stratosphere.hpp>
#include "boot_i2c_utils.hpp"

namespace ams::boot {

    namespace {

        template<typename F>
        constexpr Result RetryUntilSuccess(F f) {
            constexpr auto Timeout = TimeSpan::FromSeconds(10);
            constexpr auto RetryInterval = TimeSpan::FromMilliSeconds(20);

            TimeSpan cur_time = TimeSpan(0);
            while (true) {
                const auto retry_result = f();
                R_SUCCEED_IF(R_SUCCEEDED(retry_result));

                cur_time += RetryInterval;
                R_UNLESS(cur_time < Timeout, retry_result);

                os::SleepThread(RetryInterval);
            }
        }

    }

    Result ReadI2cRegister(i2c::driver::I2cSession &session, u8 *dst, size_t dst_size, const u8 *cmd, size_t cmd_size) {
        AMS_ABORT_UNLESS(dst != nullptr && dst_size > 0);
        AMS_ABORT_UNLESS(cmd != nullptr && cmd_size > 0);

        u8 cmd_list[i2c::CommandListLengthMax];
        i2c::CommandListFormatter formatter(cmd_list, sizeof(cmd_list));

        R_ABORT_UNLESS(formatter.EnqueueSendCommand(i2c::TransactionOption_StartCondition, cmd, cmd_size));
        R_ABORT_UNLESS(formatter.EnqueueReceiveCommand(static_cast<i2c::TransactionOption>(i2c::TransactionOption_StartCondition | i2c::TransactionOption_StopCondition), dst_size));

        return RetryUntilSuccess([&]() { return i2c::driver::ExecuteCommandList(dst, dst_size, session, cmd_list, formatter.GetCurrentLength()); });
    }

    Result WriteI2cRegister(i2c::driver::I2cSession &session, const u8 *src, size_t src_size, const u8 *cmd, size_t cmd_size) {
        AMS_ABORT_UNLESS(src != nullptr && src_size > 0);
        AMS_ABORT_UNLESS(cmd != nullptr && cmd_size > 0);

        u8 cmd_list[0x20];

        /* N doesn't use a CommandListFormatter here... */
        std::memcpy(cmd_list + 0, cmd, cmd_size);
        std::memcpy(cmd_list + cmd_size, src, src_size);

        return RetryUntilSuccess([&]() { return i2c::driver::Send(session, cmd_list, src_size + cmd_size, static_cast<i2c::TransactionOption>(i2c::TransactionOption_StartCondition | i2c::TransactionOption_StopCondition)); });
    }

    Result WriteI2cRegister(i2c::driver::I2cSession &session, const u8 address, const u8 value) {
        return WriteI2cRegister(session, std::addressof(value), sizeof(value), &address, sizeof(address));
    }

}
