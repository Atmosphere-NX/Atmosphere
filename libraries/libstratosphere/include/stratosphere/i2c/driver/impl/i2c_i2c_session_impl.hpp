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
#include <stratosphere/i2c/driver/i2c_bus_api.hpp>
#include <stratosphere/ddsf.hpp>

namespace ams::i2c::driver {

    class I2cDeviceProperty;

}

namespace ams::i2c::driver::impl {

    class I2cSessionImpl : public ::ams::ddsf::ISession {
        NON_COPYABLE(I2cSessionImpl);
        NON_MOVEABLE(I2cSessionImpl);
        AMS_DDSF_CASTABLE_TRAITS(ams::i2c::driver::impl::I2cSessionImpl, ::ams::ddsf::ISession);
        private:
            enum class Command {
                Send    = 0,
                Receive = 1,
            };
        private:
            TimeSpan retry_interval;
            int max_retry_count;
        private:
            Result SendHandler(const u8 **cur_cmd, u8 **cur_dst);
            Result ReceiveHandler(const u8 **cur_cmd, u8 **cur_dst);
            Result ExtensionHandler(const u8 **cur_cmd, u8 **cur_dst);

            Result ExecuteTransactionWithRetry(void *dst, Command command, const void *src, size_t size, TransactionOption option);
        public:
            I2cSessionImpl(int mr, TimeSpan rt) : retry_interval(rt), max_retry_count(mr) { /* ... */ }

            ~I2cSessionImpl() {
                this->Close();
            }

            Result Open(I2cDeviceProperty *device, ddsf::AccessMode access_mode);
            void Close();

            Result Send(const void *src, size_t src_size, TransactionOption option);
            Result Receive(void *dst, size_t dst_size, TransactionOption option);
            Result ExecuteCommandList(void *dst, size_t dst_size, const void *src, size_t src_size);
            Result SetRetryPolicy(int mr, int interval_us);
    };
    static_assert( sizeof(I2cSessionImpl) <= I2cSessionSize);
    static_assert(alignof(I2cSessionImpl) <= I2cSessionAlign);

    struct alignas(I2cSessionAlign) I2cSessionImplPadded {
        I2cSessionImpl _impl;
        u8 _padding[I2cSessionSize - sizeof(I2cSessionImpl)];
    };
    static_assert( sizeof(I2cSessionImplPadded) == I2cSessionSize);
    static_assert(alignof(I2cSessionImplPadded) == I2cSessionAlign);

    ALWAYS_INLINE I2cSessionImpl &GetI2cSessionImpl(I2cSession &session) {
        return GetReference(session._impl)._impl;
    }

    ALWAYS_INLINE const I2cSessionImpl &GetI2cSessionImpl(const I2cSession &session) {
        return GetReference(session._impl)._impl;
    }

    ALWAYS_INLINE I2cSessionImpl &GetOpenI2cSessionImpl(I2cSession &session) {
        auto &ref = GetReference(session._impl)._impl;
        AMS_ASSERT(ref.IsOpen());
        return ref;
    }

    ALWAYS_INLINE const I2cSessionImpl &GetOpenI2cSessionImpl(const I2cSession &session) {
        const auto &ref = GetReference(session._impl)._impl;
        AMS_ASSERT(ref.IsOpen());
        return ref;
    }

}

