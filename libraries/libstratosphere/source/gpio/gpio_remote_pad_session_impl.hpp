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
#include <stratosphere.hpp>

namespace ams::gpio {

    class RemotePadSessionImpl {
        private:
            ::GpioPadSession m_srv;
        public:
            RemotePadSessionImpl(::GpioPadSession &p) : m_srv(p) { /* ... */ }

            ~RemotePadSessionImpl() { ::gpioPadClose(std::addressof(m_srv)); }
        public:
            /* Actual commands. */
            Result SetDirection(gpio::Direction direction) {
                return ::gpioPadSetDirection(std::addressof(m_srv), static_cast<::GpioDirection>(static_cast<u32>(direction)));
            }

            Result GetDirection(ams::sf::Out<gpio::Direction> out) {
                static_assert(sizeof(gpio::Direction) == sizeof(::GpioDirection));
                return ::gpioPadGetDirection(std::addressof(m_srv), reinterpret_cast<::GpioDirection *>(out.GetPointer()));
            }

            Result SetInterruptMode(gpio::InterruptMode mode) {
                return ::gpioPadSetInterruptMode(std::addressof(m_srv), static_cast<::GpioInterruptMode>(static_cast<u32>(mode)));
            }

            Result GetInterruptMode(ams::sf::Out<gpio::InterruptMode> out) {
                static_assert(sizeof(gpio::InterruptMode) == sizeof(::GpioInterruptMode));
                return ::gpioPadGetInterruptMode(std::addressof(m_srv), reinterpret_cast<::GpioInterruptMode *>(out.GetPointer()));
            }

            Result SetInterruptEnable(bool enable) {
                return ::gpioPadSetInterruptEnable(std::addressof(m_srv), enable);
            }

            Result GetInterruptEnable(ams::sf::Out<bool> out) {
                return ::gpioPadGetInterruptEnable(std::addressof(m_srv), out.GetPointer());
            }

            Result GetInterruptStatus(ams::sf::Out<gpio::InterruptStatus> out) {
                static_assert(sizeof(gpio::InterruptStatus) == sizeof(::GpioInterruptStatus));
                return ::gpioPadGetInterruptStatus(std::addressof(m_srv), reinterpret_cast<::GpioInterruptStatus *>(out.GetPointer()));
            }

            Result ClearInterruptStatus() {
                return ::gpioPadClearInterruptStatus(std::addressof(m_srv));
            }

            Result SetValue(gpio::GpioValue value) {
                return ::gpioPadSetValue(std::addressof(m_srv), static_cast<::GpioValue>(static_cast<u32>(value)));
            }

            Result GetValue(ams::sf::Out<gpio::GpioValue> out) {
                static_assert(sizeof(gpio::GpioValue) == sizeof(::GpioValue));
                return ::gpioPadGetValue(std::addressof(m_srv), reinterpret_cast<::GpioValue *>(out.GetPointer()));
            }

            Result BindInterrupt(ams::sf::OutCopyHandle out) {
                ::Event ev;
                R_TRY(::gpioPadBindInterrupt(std::addressof(m_srv), std::addressof(ev)));
                out.SetValue(ev.revent, true);
                return ResultSuccess();
            }

            Result UnbindInterrupt() {
                return ::gpioPadUnbindInterrupt(std::addressof(m_srv));
            }

            Result SetDebounceEnabled(bool enable) {
                return ::gpioPadSetDebounceEnabled(std::addressof(m_srv), enable);
            }

            Result GetDebounceEnabled(ams::sf::Out<bool> out) {
                return ::gpioPadGetDebounceEnabled(std::addressof(m_srv), out.GetPointer());
            }

            Result SetDebounceTime(s32 ms) {
                return ::gpioPadSetDebounceTime(std::addressof(m_srv), ms);
            }

            Result GetDebounceTime(ams::sf::Out<s32> out) {
                return ::gpioPadGetDebounceTime(std::addressof(m_srv), out.GetPointer());
            }

            Result SetValueForSleepState(gpio::GpioValue value) {
                /* TODO: libnx bindings. */
                AMS_UNUSED(value);
                AMS_ABORT();
            }

            Result GetValueForSleepState(ams::sf::Out<gpio::GpioValue> out) {
                /* TODO: libnx bindings. */
                AMS_UNUSED(out);
                AMS_ABORT();
            }
    };
    static_assert(gpio::sf::IsIPadSession<RemotePadSessionImpl>);

}
