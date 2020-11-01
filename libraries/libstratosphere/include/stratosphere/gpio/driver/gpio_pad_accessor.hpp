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
#include <stratosphere/gpio/gpio_types.hpp>

namespace ams::gpio::driver {

    namespace impl {

        constexpr inline size_t GpioPadSessionSize  = 0x60;
        constexpr inline size_t GpioPadSessionAlign = 8;
        struct alignas(GpioPadSessionAlign) GpioPadSessionImplPadded;

    }

    struct GpioPadSession {
        util::TypedStorage<impl::GpioPadSessionImplPadded, impl::GpioPadSessionSize, impl::GpioPadSessionAlign> _impl;
    };

    Result OpenSession(GpioPadSession *out, DeviceCode device_code, ddsf::AccessMode access_mode);
    void CloseSession(GpioPadSession *session);

    Result SetDirection(GpioPadSession *session, gpio::Direction direction);
    Result GetDirection(gpio::Direction *out, GpioPadSession *session);

    Result SetValue(GpioPadSession *session, gpio::GpioValue value);
    Result GetValue(gpio::GpioValue *out, GpioPadSession *session);

    /* TODO */

}
