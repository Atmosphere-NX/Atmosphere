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
#include "i2c_api.hpp"
#include "impl/i2c_resource_manager.hpp"

namespace ams::i2c::driver {

    namespace {

        /* For convenience. */
        using CommandHandler = Result (*)(const u8 **cur_cmd, u8 **cur_dst, Session& session);

        /* Command handlers. */
        Result SendHandler(const u8 **cur_cmd, u8 **cur_dst, Session& session) {
            I2cTransactionOption option = static_cast<I2cTransactionOption>(
                (((**cur_cmd) & (1 << 6)) ? I2cTransactionOption_Start : 0) | (((**cur_cmd) & (1 << 7)) ? I2cTransactionOption_Stop : 0)
            );
            (*cur_cmd)++;

            size_t num_bytes = (**cur_cmd);
            (*cur_cmd)++;

            R_TRY(Send(session, *cur_cmd, num_bytes, option));
            (*cur_cmd) += num_bytes;

            return ResultSuccess();
        }

        Result ReceiveHandler(const u8 **cur_cmd, u8 **cur_dst, Session& session) {
            I2cTransactionOption option = static_cast<I2cTransactionOption>(
                (((**cur_cmd) & (1 << 6)) ? I2cTransactionOption_Start : 0) | (((**cur_cmd) & (1 << 7)) ? I2cTransactionOption_Stop : 0)
            );
            (*cur_cmd)++;

            size_t num_bytes = (**cur_cmd);
            (*cur_cmd)++;

            R_TRY(Receive(session, *cur_dst, num_bytes, option));
            (*cur_dst) += num_bytes;

            return ResultSuccess();
        }

        Result SubCommandHandler(const u8 **cur_cmd, u8 **cur_dst, Session& session) {
            const SubCommand sub_cmd = static_cast<SubCommand>((**cur_cmd) >> 2);
            (*cur_cmd)++;

            switch (sub_cmd) {
                case SubCommand::Sleep:
                    {
                        const size_t us = (**cur_cmd);
                        (*cur_cmd)++;
                        svcSleepThread(us * 1'000ul);
                    }
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
            return ResultSuccess();
        }

        /* Command handler list. */
        constexpr CommandHandler g_cmd_handlers[static_cast<size_t>(Command::Count)] = {
            SendHandler,
            ReceiveHandler,
            SubCommandHandler,
        };

        inline impl::ResourceManager &GetResourceManager() {
            return impl::ResourceManager::GetInstance();
        }

        inline void CheckInitialized() {
            AMS_ABORT_UNLESS(GetResourceManager().IsInitialized());
        }

    }

    /* Initialization. */
    void Initialize() {
        GetResourceManager().Initialize();
    }

    void Finalize() {
        GetResourceManager().Finalize();
    }

    /* Session management. */
    void OpenSession(Session *out_session, I2cDevice device) {
        CheckInitialized();
        AMS_ABORT_UNLESS(impl::IsDeviceSupported(device));

        const auto bus = impl::GetDeviceBus(device);
        const auto slave_address = impl::GetDeviceSlaveAddress(device);
        const auto addressing_mode = impl::GetDeviceAddressingMode(device);
        const auto speed_mode = impl::GetDeviceSpeedMode(device);
        const auto max_retries = impl::GetDeviceMaxRetries(device);
        const auto retry_wait_time = impl::GetDeviceRetryWaitTime(device);
        GetResourceManager().OpenSession(out_session, bus, slave_address, addressing_mode, speed_mode, max_retries, retry_wait_time);
    }

    void CloseSession(Session &session) {
        CheckInitialized();
        GetResourceManager().CloseSession(session);
    }

    /* Communication. */
    Result Send(Session &session, const void *src, size_t size, I2cTransactionOption option) {
        CheckInitialized();
        AMS_ABORT_UNLESS(src != nullptr);
        AMS_ABORT_UNLESS(size > 0);

        std::scoped_lock<os::Mutex &> lk(GetResourceManager().GetTransactionMutex(impl::ConvertFromIndex(session.bus_idx)));
        return GetResourceManager().GetSession(session.session_id).DoTransactionWithRetry(nullptr, src, size, option, impl::Command::Send);
    }

    Result Receive(Session &session, void *dst, size_t size, I2cTransactionOption option) {
        CheckInitialized();
        AMS_ABORT_UNLESS(dst != nullptr);
        AMS_ABORT_UNLESS(size > 0);

        std::scoped_lock<os::Mutex &> lk(GetResourceManager().GetTransactionMutex(impl::ConvertFromIndex(session.bus_idx)));
        return GetResourceManager().GetSession(session.session_id).DoTransactionWithRetry(dst, nullptr, size, option, impl::Command::Receive);
    }

    Result ExecuteCommandList(Session &session, void *dst, size_t size, const void *cmd_list, size_t cmd_list_size) {
        CheckInitialized();
        AMS_ABORT_UNLESS(dst != nullptr && size > 0);
        AMS_ABORT_UNLESS(cmd_list != nullptr && cmd_list_size > 0);

        u8 *cur_dst = static_cast<u8 *>(dst);
        const u8 *cur_cmd = static_cast<const u8 *>(cmd_list);
        const u8 *cmd_list_end = cur_cmd + cmd_list_size;

        while (cur_cmd < cmd_list_end) {
            Command cmd = static_cast<Command>((*cur_cmd) & 3);
            AMS_ABORT_UNLESS(cmd < Command::Count);

            R_TRY(g_cmd_handlers[static_cast<size_t>(cmd)](&cur_cmd, &cur_dst, session));
        }

        return ResultSuccess();
    }

    /* Power management. */
    void SuspendBuses() {
        GetResourceManager().SuspendBuses();
    }

    void ResumeBuses() {
        GetResourceManager().ResumeBuses();
    }

    void SuspendPowerBus() {
        GetResourceManager().SuspendPowerBus();
    }

    void ResumePowerBus() {
        GetResourceManager().ResumePowerBus();
    }

}
