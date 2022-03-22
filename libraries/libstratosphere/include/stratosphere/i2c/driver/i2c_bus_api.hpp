/*
 * Copyright (c) Atmosphère-NX
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

namespace ams::i2c::driver {

    namespace impl {

        constexpr inline size_t I2cSessionSize  = 0x60;
        constexpr inline size_t I2cSessionAlign = 8;
        struct alignas(I2cSessionAlign) I2cSessionImplPadded;

    }

    struct I2cSession {
        util::TypedStorage<impl::I2cSessionImplPadded, impl::I2cSessionSize, impl::I2cSessionAlign> _impl;
    };

    Result OpenSession(I2cSession *out, DeviceCode device_code);
    void CloseSession(I2cSession &session);

    Result Send(I2cSession &session, const void *src, size_t src_size, TransactionOption option);
    Result Receive(void *dst, size_t dst_size, I2cSession &session, TransactionOption option);

    Result ExecuteCommandList(void *dst, size_t dst_size, I2cSession &session, const void *src, size_t src_size);

    Result SetRetryPolicy(I2cSession &session, int max_retry_count, int retry_interval_us);

}

