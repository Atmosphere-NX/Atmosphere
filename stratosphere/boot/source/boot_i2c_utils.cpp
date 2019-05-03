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
#include "i2c_driver/i2c_api.hpp"

template<typename F>
static Result RetryUntilSuccess(F f) {
    constexpr u64 timeout = 10'000'000'000ul;
    constexpr u64 retry_interval = 20'000'000ul;

    u64 cur_time = 0;
    while (true) {
        Result rc = f();
        if (R_SUCCEEDED(rc)) {
            return rc;
        } else {
            cur_time += retry_interval;
            if (cur_time >= timeout) {
                return rc;
            }
        }
        svcSleepThread(retry_interval);
    }
}

Result Boot::ReadI2cRegister(I2cSessionImpl &session, u8 *dst, size_t dst_size, const u8 *cmd, size_t cmd_size) {
    Result rc;
    if (dst == nullptr || dst_size == 0 || cmd == nullptr || cmd_size == 0) {
        std::abort();
    }

    u8 cmd_list[I2cCommandListFormatter::MaxCommandListSize];

    I2cCommandListFormatter formatter(cmd_list, sizeof(cmd_list));
    if (R_FAILED((rc = formatter.EnqueueSendCommand(I2cTransactionOption_Start, cmd, cmd_size)))) {
        std::abort();
    }
    if (R_FAILED((rc = formatter.EnqueueReceiveCommand(static_cast<I2cTransactionOption>(I2cTransactionOption_Start | I2cTransactionOption_Stop), dst_size)))) {
        std::abort();
    }

    return RetryUntilSuccess([&]() { return I2cDriver::ExecuteCommandList(session, dst, dst_size, cmd_list, formatter.GetCurrentSize()); });
}

Result Boot::WriteI2cRegister(I2cSessionImpl &session, const u8 *src, size_t src_size, const u8 *cmd, size_t cmd_size) {
    if (src == nullptr || src_size == 0 || cmd == nullptr || cmd_size == 0) {
        std::abort();
    }

    u8 cmd_list[0x20];

    /* N doesn't use a CommandListFormatter here... */
    std::memcpy(&cmd_list[0], cmd, cmd_size);
    std::memcpy(&cmd_list[cmd_size], src, src_size);

    return RetryUntilSuccess([&]() { return I2cDriver::Send(session, cmd_list, src_size + cmd_size, static_cast<I2cTransactionOption>(I2cTransactionOption_Start | I2cTransactionOption_Stop)); });
}

Result Boot::WriteI2cRegister(I2cSessionImpl &session, const u8 address, const u8 value) {
    return Boot::WriteI2cRegister(session, &value, sizeof(value), &address, sizeof(address));
}
