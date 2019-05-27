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

#include <switch.h>
#include <stratosphere.hpp>

#include "i2c_api.hpp"
#include "i2c_resource_manager.hpp"

typedef Result (*CommandHandler)(const u8 **cur_cmd, u8 **cur_dst, I2cSessionImpl& session);

static Result I2cSendHandler(const u8 **cur_cmd, u8 **cur_dst, I2cSessionImpl& session) {
    I2cTransactionOption option = static_cast<I2cTransactionOption>(
        (((**cur_cmd) & (1 << 6)) ? I2cTransactionOption_Start : 0) | (((**cur_cmd) & (1 << 7)) ? I2cTransactionOption_Stop : 0)
    );
    (*cur_cmd)++;

    size_t num_bytes = (**cur_cmd);
    (*cur_cmd)++;

    Result rc = I2cDriver::Send(session, *cur_cmd, num_bytes, option);
    if (R_FAILED(rc)) {
        return rc;
    }
    (*cur_cmd) += num_bytes;

    return ResultSuccess;
}

static Result I2cReceiveHandler(const u8 **cur_cmd, u8 **cur_dst, I2cSessionImpl& session) {
    I2cTransactionOption option = static_cast<I2cTransactionOption>(
        (((**cur_cmd) & (1 << 6)) ? I2cTransactionOption_Start : 0) | (((**cur_cmd) & (1 << 7)) ? I2cTransactionOption_Stop : 0)
    );
    (*cur_cmd)++;

    size_t num_bytes = (**cur_cmd);
    (*cur_cmd)++;

    Result rc = I2cDriver::Receive(session, *cur_dst, num_bytes, option);
    if (R_FAILED(rc)) {
        return rc;
    }
    (*cur_dst) += num_bytes;

    return ResultSuccess;
}

static Result I2cSubCommandHandler(const u8 **cur_cmd, u8 **cur_dst, I2cSessionImpl& session) {
    const I2cSubCommand sub_cmd = static_cast<I2cSubCommand>((**cur_cmd) >> 2);
    (*cur_cmd)++;

    switch (sub_cmd) {
        case I2cSubCommand_Sleep:
            {
                const size_t us = (**cur_cmd);
                (*cur_cmd)++;
                svcSleepThread(us * 1'000ul);
            }
            break;
        default:
            std::abort();
    }
    return ResultSuccess;
}

static constexpr CommandHandler g_cmd_handlers[I2cCommand_Count] = {
    I2cSendHandler,
    I2cReceiveHandler,
    I2cSubCommandHandler,
};

static inline I2cResourceManager &GetResourceManager() {
    return I2cResourceManager::GetInstance();
}

static inline void CheckInitialized() {
    if (!GetResourceManager().IsInitialized()) {
        std::abort();
    }
}

void I2cDriver::Initialize() {
    GetResourceManager().Initialize();
}

void I2cDriver::Finalize() {
    GetResourceManager().Finalize();
}

void I2cDriver::OpenSession(I2cSessionImpl *out_session, I2cDevice device) {
    CheckInitialized();
    if (!IsI2cDeviceSupported(device)) {
        std::abort();
    }

    const I2cBus bus = GetI2cDeviceBus(device);
    const u32 slave_address = GetI2cDeviceSlaveAddress(device);
    const AddressingMode addressing_mode = GetI2cDeviceAddressingMode(device);
    const SpeedMode speed_mode = GetI2cDeviceSpeedMode(device);
    const u32 max_retries = GetI2cDeviceMaxRetries(device);
    const u64 retry_wait_time = GetI2cDeviceRetryWaitTime(device);
    GetResourceManager().OpenSession(out_session, bus, slave_address, addressing_mode, speed_mode, max_retries, retry_wait_time);
}

void I2cDriver::CloseSession(I2cSessionImpl &session) {
    CheckInitialized();
    GetResourceManager().CloseSession(session);
}

Result I2cDriver::Send(I2cSessionImpl &session, const void *src, size_t size, I2cTransactionOption option) {
    CheckInitialized();
    if (src == nullptr || size == 0) {
        std::abort();
    }

    std::scoped_lock<HosMutex &> lk(GetResourceManager().GetTransactionMutex(session.bus));
    return GetResourceManager().GetSession(session.session_id).DoTransactionWithRetry(nullptr, src, size, option, DriverCommand_Send);
}

Result I2cDriver::Receive(I2cSessionImpl &session, void *dst, size_t size, I2cTransactionOption option) {
    CheckInitialized();
    if (dst == nullptr || size == 0) {
        std::abort();
    }

    std::scoped_lock<HosMutex &> lk(GetResourceManager().GetTransactionMutex(session.bus));
    return GetResourceManager().GetSession(session.session_id).DoTransactionWithRetry(dst, nullptr, size, option, DriverCommand_Receive);
}

Result I2cDriver::ExecuteCommandList(I2cSessionImpl &session, void *dst, size_t size, const void *cmd_list, size_t cmd_list_size) {
    CheckInitialized();
    if (dst == nullptr || size == 0 || cmd_list == nullptr || cmd_list_size == 0) {
        std::abort();
    }

    u8 *cur_dst = static_cast<u8 *>(dst);
    const u8 *cur_cmd = static_cast<const u8 *>(cmd_list);
    const u8 *cmd_list_end = cur_cmd + cmd_list_size;

    while (cur_cmd < cmd_list_end) {
        I2cCommand cmd = static_cast<I2cCommand>((*cur_cmd) & 3);
        if (cmd >= I2cCommand_Count) {
            std::abort();
        }

        Result rc = g_cmd_handlers[cmd](&cur_cmd, &cur_dst, session);
        if (R_FAILED(rc)) {
            return rc;
        }
    }

    return ResultSuccess;
}

void I2cDriver::SuspendBuses() {
    GetResourceManager().SuspendBuses();
}

void I2cDriver::ResumeBuses() {
    GetResourceManager().ResumeBuses();
}

void I2cDriver::SuspendPowerBus() {
    GetResourceManager().SuspendPowerBus();
}

void I2cDriver::ResumePowerBus() {
    GetResourceManager().ResumePowerBus();
}