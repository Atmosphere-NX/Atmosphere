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
#include "i2c_driver_core.hpp"
#include "../../impl/i2c_command_list_format.hpp"

namespace ams::i2c::driver::impl {

    namespace {

        constexpr TransactionOption EncodeTransactionOption(bool start, bool stop) {
            return static_cast<TransactionOption>((start ? TransactionOption_StartCondition : 0) | (stop ? TransactionOption_StopCondition : 0));
        }

    }

    Result I2cSessionImpl::Open(I2cDeviceProperty *device, ddsf::AccessMode access_mode) {
        AMS_ASSERT(device != nullptr);

        /* Check if we're the device's first session. */
        const bool first = !device->HasAnyOpenSession();

        /* Open the session. */
        R_TRY(ddsf::OpenSession(device, this, access_mode));
        auto guard = SCOPE_GUARD { ddsf::CloseSession(this); };

        /* If we're the first session, initialize the device. */
        if (first) {
            R_TRY(device->GetDriver().SafeCastTo<II2cDriver>().InitializeDevice(device));
        }

        /* We're opened. */
        guard.Cancel();
        return ResultSuccess();
    }

    void I2cSessionImpl::Close() {
        /* If we're not open, do nothing. */
        if (!this->IsOpen()) {
            return;
        }

        /* Get the device. */
        auto &device = this->GetDevice().SafeCastTo<I2cDeviceProperty>();

        /* Close the session. */
        ddsf::CloseSession(this);

        /* If there are no remaining sessions, finalize the device. */
        if (!device.HasAnyOpenSession()) {
            device.GetDriver().SafeCastTo<II2cDriver>().FinalizeDevice(std::addressof(device));
        }
    }

    Result I2cSessionImpl::SendHandler(const u8 **cur_cmd, u8 **cur_dst) {
        /* Read the header bytes. */
        const util::BitPack8 hdr0{*((*cur_cmd)++)};
        const util::BitPack8 hdr1{*((*cur_cmd)++)};

        /* Decode the header. */
        const bool start  = hdr0.Get<i2c::impl::SendCommandFormat::StartCondition>();
        const bool stop   = hdr0.Get<i2c::impl::SendCommandFormat::StopCondition>();
        const size_t size = hdr1.Get<i2c::impl::SendCommandFormat::Size>();

        /* Execute the transaction. */
        R_TRY(this->ExecuteTransactionWithRetry(nullptr, Command::Send, *cur_cmd, size, EncodeTransactionOption(start, stop)));

        /* Advance. */
        *cur_cmd += size;

        return ResultSuccess();
    }

    Result I2cSessionImpl::ReceiveHandler(const u8 **cur_cmd, u8 **cur_dst) {
        /* Read the header bytes. */
        const util::BitPack8 hdr0{*((*cur_cmd)++)};
        const util::BitPack8 hdr1{*((*cur_cmd)++)};

        /* Decode the header. */
        const bool start  = hdr0.Get<i2c::impl::ReceiveCommandFormat::StartCondition>();
        const bool stop   = hdr0.Get<i2c::impl::ReceiveCommandFormat::StopCondition>();
        const size_t size = hdr1.Get<i2c::impl::ReceiveCommandFormat::Size>();

        /* Execute the transaction. */
        R_TRY(this->ExecuteTransactionWithRetry(*cur_dst, Command::Receive, nullptr, size, EncodeTransactionOption(start, stop)));

        /* Advance. */
        *cur_dst += size;

        return ResultSuccess();
    }

    Result I2cSessionImpl::ExtensionHandler(const u8 **cur_cmd, u8 **cur_dst) {
        /* Read the header bytes. */
        const util::BitPack8 hdr0{*((*cur_cmd)++)};

        /* Execute the subcommand. */
        switch (hdr0.Get<i2c::impl::CommonCommandFormat::SubCommandId>()) {
            case i2c::impl::SubCommandId_Sleep:
                {
                    const util::BitPack8 param{*((*cur_cmd)++)};

                    os::SleepThread(TimeSpan::FromMicroSeconds(param.Get<i2c::impl::SleepCommandFormat::MicroSeconds>()));
                }
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        return ResultSuccess();
    }

    Result I2cSessionImpl::ExecuteTransactionWithRetry(void *dst, Command command, const void *src, size_t size, TransactionOption option) {
        /* Get the device. */
        auto &device = GetDevice().SafeCastTo<I2cDeviceProperty>();

        /* Repeatedly try to execute the transaction. */
        int retry_count;
        while (true) {
            /* Execute the transaction. */
            Result result;
            switch (command) {
                case Command::Send:    result = device.GetDriver().SafeCastTo<II2cDriver>().Send(std::addressof(device), src, size, option); break;
                case Command::Receive: result = device.GetDriver().SafeCastTo<II2cDriver>().Receive(dst, size, std::addressof(device), option); break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            /* If we timed out, retry up to our max retry count. */
            R_TRY_CATCH(result) {
                R_CATCH(i2c::ResultTimeout) {
                    if ((++retry_count) <= this->max_retry_count) {
                        os::SleepThread(this->retry_interval);
                        continue;
                    }
                    return i2c::ResultBusBusy();
                }
            } R_END_TRY_CATCH;

            return ResultSuccess();
        }
    }

    Result I2cSessionImpl::Send(const void *src, size_t src_size, TransactionOption option) {
        /* Acquire exclusive access to the device. */
        std::scoped_lock lk(this->GetDevice().SafeCastTo<I2cDeviceProperty>().GetDriver().SafeCastTo<II2cDriver>().GetTransactionOrderMutex());

        return this->ExecuteTransactionWithRetry(nullptr, Command::Send, src, src_size, option);
    }

    Result I2cSessionImpl::Receive(void *dst, size_t dst_size, TransactionOption option) {
        /* Acquire exclusive access to the device. */
        std::scoped_lock lk(this->GetDevice().SafeCastTo<I2cDeviceProperty>().GetDriver().SafeCastTo<II2cDriver>().GetTransactionOrderMutex());

        return this->ExecuteTransactionWithRetry(dst, Command::Receive, nullptr, dst_size, option);
    }

    Result I2cSessionImpl::ExecuteCommandList(void *dst, size_t dst_size, const void *src, size_t src_size) {
        /* Acquire exclusive access to the device. */
        std::scoped_lock lk(this->GetDevice().SafeCastTo<I2cDeviceProperty>().GetDriver().SafeCastTo<II2cDriver>().GetTransactionOrderMutex());

        /* Prepare to process the command list. */
        const u8 *       cur_u8 = static_cast<const u8 *>(src);
        const u8 * const end_u8 = cur_u8 + src_size;
              u8 *       dst_u8 = static_cast<u8 *>(dst);

        /* Process commands. */
        while (cur_u8 < end_u8) {
            const util::BitPack8 hdr{*cur_u8};

            switch (hdr.Get<i2c::impl::CommonCommandFormat::CommandId>()) {
                case i2c::impl::CommandId_Send:      R_TRY(this->SendHandler(std::addressof(cur_u8), std::addressof(dst_u8)));      break;
                case i2c::impl::CommandId_Receive:   R_TRY(this->ReceiveHandler(std::addressof(cur_u8), std::addressof(dst_u8)));   break;
                case i2c::impl::CommandId_Extension: R_TRY(this->ExtensionHandler(std::addressof(cur_u8), std::addressof(dst_u8))); break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        return ResultSuccess();
    }

    Result I2cSessionImpl::SetRetryPolicy(int mr, int interval_us) {
        this->max_retry_count = mr;
        this->retry_interval  = TimeSpan::FromMicroSeconds(interval_us);
        return ResultSuccess();
    }

}
