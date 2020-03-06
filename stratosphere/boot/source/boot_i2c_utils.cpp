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
#include "boot_i2c_utils.hpp"

namespace ams::boot {

    namespace {

        template<typename F>
        constexpr Result RetryUntilSuccess(F f) {
            constexpr u64 timeout = 10'000'000'000ul;
            constexpr u64 retry_interval = 20'000'000ul;

            u64 cur_time = 0;
            while (true) {
                const auto retry_result = f();
                R_SUCCEED_IF(R_SUCCEEDED(retry_result));

                cur_time += retry_interval;
                if (cur_time < timeout) {
                    svcSleepThread(retry_interval);
                    continue;
                }

                return retry_result;
            }
        }

    }

    Result ReadI2cRegister(i2c::driver::Session &session, u8 *dst, size_t dst_size, const u8 *cmd, size_t cmd_size) {
        AMS_ABORT_UNLESS(dst != nullptr && dst_size > 0);
        AMS_ABORT_UNLESS(cmd != nullptr && cmd_size > 0);

        u8 cmd_list[i2c::CommandListFormatter::MaxCommandListSize];

        i2c::CommandListFormatter formatter(cmd_list, sizeof(cmd_list));
        R_ABORT_UNLESS(formatter.EnqueueSendCommand(I2cTransactionOption_Start, cmd, cmd_size));
        R_ABORT_UNLESS(formatter.EnqueueReceiveCommand(static_cast<I2cTransactionOption>(I2cTransactionOption_Start | I2cTransactionOption_Stop), dst_size));

        return RetryUntilSuccess([&]() { return i2c::driver::ExecuteCommandList(session, dst, dst_size, cmd_list, formatter.GetCurrentSize()); });
    }

    Result WriteI2cRegister(i2c::driver::Session &session, const u8 *src, size_t src_size, const u8 *cmd, size_t cmd_size) {
        AMS_ABORT_UNLESS(src != nullptr && src_size > 0);
        AMS_ABORT_UNLESS(cmd != nullptr && cmd_size > 0);

        u8 cmd_list[0x20];

        /* N doesn't use a CommandListFormatter here... */
        std::memcpy(&cmd_list[0], cmd, cmd_size);
        std::memcpy(&cmd_list[cmd_size], src, src_size);

        return RetryUntilSuccess([&]() { return i2c::driver::Send(session, cmd_list, src_size + cmd_size, static_cast<I2cTransactionOption>(I2cTransactionOption_Start | I2cTransactionOption_Stop)); });
    }

    Result WriteI2cRegister(i2c::driver::Session &session, const u8 address, const u8 value) {
        return WriteI2cRegister(session, &value, sizeof(value), &address, sizeof(address));
    }

}
