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
#include <stratosphere.hpp>

namespace ams::gpio::server {

    class ManagerImpl;

    class PadSessionImpl {
        private:
            ManagerImpl *parent; /* NOTE: this is an sf::SharedPointer<> in Nintendo's code. */
            gpio::driver::GpioPadSession internal_pad_session;
            bool has_session;
            os::SystemEvent system_event;
        public:
            explicit PadSessionImpl(ManagerImpl *p) : parent(p), has_session(false) { /* ... */ }

            ~PadSessionImpl() {
                if (this->has_session) {
                    gpio::driver::CloseSession(std::addressof(this->internal_pad_session));
                }
            }

            Result OpenSession(DeviceCode device_code, ddsf::AccessMode access_mode) {
                AMS_ABORT_UNLESS(!this->has_session);

                R_TRY(gpio::driver::OpenSession(std::addressof(this->internal_pad_session), device_code, access_mode));
                this->has_session = true;
                return ResultSuccess();
            }
        public:
            /* Actual commands. */
            Result SetDirection(gpio::Direction direction) {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* Validate the direction. */
                R_UNLESS((direction == Direction_Input || direction == Direction_Output), gpio::ResultInvalidArgument());

                /* Invoke the driver library. */
                R_TRY(gpio::driver::SetDirection(std::addressof(this->internal_pad_session), direction));

                return ResultSuccess();
            }

            Result GetDirection(ams::sf::Out<gpio::Direction> out) {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* Invoke the driver library. */
                R_TRY(gpio::driver::GetDirection(out.GetPointer(), std::addressof(this->internal_pad_session)));

                return ResultSuccess();
            }

            Result SetInterruptMode(gpio::InterruptMode mode) {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* TODO */
                AMS_ABORT();
            }

            Result GetInterruptMode(ams::sf::Out<gpio::InterruptMode> out) {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* TODO */
                AMS_ABORT();
            }

            Result SetInterruptEnable(bool enable) {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* TODO */
                AMS_ABORT();
            }

            Result GetInterruptEnable(ams::sf::Out<bool> out) {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* TODO */
                AMS_ABORT();
            }

            Result GetInterruptStatus(ams::sf::Out<gpio::InterruptStatus> out) {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* TODO */
                AMS_ABORT();
            }

            Result ClearInterruptStatus() {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* TODO */
                AMS_ABORT();
            }

            Result SetValue(gpio::GpioValue value) {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* Validate the value. */
                R_UNLESS((value == GpioValue_Low || value == GpioValue_High), gpio::ResultInvalidArgument());

                /* Invoke the driver library. */
                R_TRY(gpio::driver::SetValue(std::addressof(this->internal_pad_session), value));

                return ResultSuccess();
            }

            Result GetValue(ams::sf::Out<gpio::GpioValue> out) {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* Invoke the driver library. */
                R_TRY(gpio::driver::GetValue(out.GetPointer(), std::addressof(this->internal_pad_session)));

                return ResultSuccess();
            }

            Result BindInterrupt(ams::sf::OutCopyHandle out) {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* TODO */
                AMS_ABORT();
            }

            Result UnbindInterrupt() {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* TODO */
                AMS_ABORT();
            }

            Result SetDebounceEnabled(bool enable) {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* TODO */
                AMS_ABORT();
            }

            Result GetDebounceEnabled(ams::sf::Out<bool> out) {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* TODO */
                AMS_ABORT();
            }

            Result SetDebounceTime(s32 ms) {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* TODO */
                AMS_ABORT();
            }

            Result GetDebounceTime(ams::sf::Out<s32> out) {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* TODO */
                AMS_ABORT();
            }

            Result SetValueForSleepState(gpio::GpioValue value) {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* TODO */
                AMS_ABORT();
            }

            Result GetValueForSleepState(ams::sf::Out<gpio::GpioValue> out) {
                /* Validate our state. */
                AMS_ASSERT(this->has_session);

                /* TODO */
                AMS_ABORT();
            }
    };
    static_assert(gpio::sf::IsIPadSession<PadSessionImpl>);

}
