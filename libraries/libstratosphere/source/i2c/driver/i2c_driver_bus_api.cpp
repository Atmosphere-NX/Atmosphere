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

#include "impl/i2c_driver_core.hpp"

namespace ams::i2c::driver {

    namespace {

        constexpr inline int DefaultRetryCount = 3;
        constexpr inline TimeSpan DefaultRetryInterval = TimeSpan::FromMilliSeconds(5);

        Result OpenSessionImpl(I2cSession *out, I2cDeviceProperty *device) {
            /* Construct the session. */
            auto *session = new (std::addressof(impl::GetI2cSessionImpl(*out))) impl::I2cSessionImpl(DefaultRetryCount, DefaultRetryInterval);
            auto session_guard = SCOPE_GUARD { session->~I2cSessionImpl(); };

            /* Open the session. */
            R_TRY(session->Open(device, ddsf::AccessMode_ReadWrite));

            /* We succeeded. */
            session_guard.Cancel();
            return ResultSuccess();
        }

    }

    Result OpenSession(I2cSession *out, DeviceCode device_code) {
        AMS_ASSERT(out != nullptr);

        /* Find the device. */
        I2cDeviceProperty *device = nullptr;
        R_TRY(impl::FindDevice(std::addressof(device), device_code));
        AMS_ASSERT(device != nullptr);

        /* Open the session. */
        R_TRY(OpenSessionImpl(out, device));

        return ResultSuccess();
    }

    void CloseSession(I2cSession &session) {
        impl::GetOpenI2cSessionImpl(session).~I2cSessionImpl();
    }

    Result Send(I2cSession &session, const void *src, size_t src_size, TransactionOption option) {
        AMS_ASSERT(src != nullptr);
        AMS_ABORT_UNLESS(src_size > 0);

        return impl::GetOpenI2cSessionImpl(session).Send(src, src_size, option);
    }

    Result Receive(void *dst, size_t dst_size, I2cSession &session, TransactionOption option) {
        AMS_ASSERT(dst != nullptr);
        AMS_ABORT_UNLESS(dst_size > 0);

        return impl::GetOpenI2cSessionImpl(session).Receive(dst, dst_size, option);
    }

    Result ExecuteCommandList(void *dst, size_t dst_size, I2cSession &session, const void *src, size_t src_size) {
        AMS_ASSERT(src != nullptr);
        AMS_ASSERT(dst != nullptr);

        AMS_ABORT_UNLESS(src_size > 0);
        AMS_ABORT_UNLESS(dst_size > 0);

        return impl::GetOpenI2cSessionImpl(session).ExecuteCommandList(dst, dst_size, src, src_size);
    }

    Result SetRetryPolicy(I2cSession &session, int max_retry_count, int retry_interval_us) {
        AMS_ASSERT(max_retry_count > 0);
        AMS_ASSERT(retry_interval_us > 0);

        return impl::GetOpenI2cSessionImpl(session).SetRetryPolicy(max_retry_count, retry_interval_us);
    }

}
