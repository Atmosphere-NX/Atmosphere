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
#include <stratosphere/ddsf/ddsf_types.hpp>
#include <stratosphere/gpio/gpio_types.hpp>
#include <stratosphere/gpio/gpio_select_pad_name.hpp>
#include <stratosphere/gpio/driver/gpio_pad.hpp>

namespace ams::gpio {

    struct GpioPadSession {
        void *_session;
        os::SystemEventType *_event;
    };

    Result OpenSession(GpioPadSession *out_session, ams::DeviceCode device_code);
    void CloseSession(GpioPadSession *session);

    Direction GetDirection(GpioPadSession *session);
    void SetDirection(GpioPadSession *session, Direction direction);

    GpioValue GetValue(GpioPadSession *session);
    void SetValue(GpioPadSession *session, GpioValue value);

    InterruptMode GetInterruptMode(GpioPadSession *session);
    void SetInterruptMode(GpioPadSession *session, InterruptMode mode);

    bool GetInterruptEnable(GpioPadSession *session);
    void SetInterruptEnable(GpioPadSession *session, bool en);

    InterruptStatus GetInterruptStatus(GpioPadSession *session);
    void ClearInterruptStatus(GpioPadSession *session);

    int GetDebounceTime(GpioPadSession *session);
    void SetDebounceTime(GpioPadSession *session, int ms);

    bool GetDebounceEnabled(GpioPadSession *session);
    void SetDebounceEnabled(GpioPadSession *session, bool en);

    Result BindInterrupt(os::SystemEventType *event, GpioPadSession *session);
    void UnbindInterrupt(GpioPadSession *session);

    Result IsWakeEventActive(bool *out_is_active, ams::DeviceCode device_code);

}
