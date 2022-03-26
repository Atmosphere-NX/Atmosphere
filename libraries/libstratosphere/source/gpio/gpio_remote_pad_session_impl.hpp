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

    #if defined(ATMOSPHERE_OS_HORIZON)
    class RemotePadSessionImpl {
        private:
            ::GpioPadSession m_srv;
        public:
            RemotePadSessionImpl(::GpioPadSession &p) : m_srv(p) { /* ... */ }

            ~RemotePadSessionImpl() { ::gpioPadClose(std::addressof(m_srv)); }
        public:
            /* Actual commands. */
            Result SetDirection(gpio::Direction direction) {
                R_RETURN(::gpioPadSetDirection(std::addressof(m_srv), static_cast<::GpioDirection>(static_cast<u32>(direction))));
            }

            Result GetDirection(ams::sf::Out<gpio::Direction> out) {
                static_assert(sizeof(gpio::Direction) == sizeof(::GpioDirection));
                R_RETURN(::gpioPadGetDirection(std::addressof(m_srv), reinterpret_cast<::GpioDirection *>(out.GetPointer())));
            }

            Result SetInterruptMode(gpio::InterruptMode mode) {
                R_RETURN(::gpioPadSetInterruptMode(std::addressof(m_srv), static_cast<::GpioInterruptMode>(static_cast<u32>(mode))));
            }

            Result GetInterruptMode(ams::sf::Out<gpio::InterruptMode> out) {
                static_assert(sizeof(gpio::InterruptMode) == sizeof(::GpioInterruptMode));
                R_RETURN(::gpioPadGetInterruptMode(std::addressof(m_srv), reinterpret_cast<::GpioInterruptMode *>(out.GetPointer())));
            }

            Result SetInterruptEnable(bool enable) {
                R_RETURN(::gpioPadSetInterruptEnable(std::addressof(m_srv), enable));
            }

            Result GetInterruptEnable(ams::sf::Out<bool> out) {
                R_RETURN(::gpioPadGetInterruptEnable(std::addressof(m_srv), out.GetPointer()));
            }

            Result GetInterruptStatus(ams::sf::Out<gpio::InterruptStatus> out) {
                static_assert(sizeof(gpio::InterruptStatus) == sizeof(::GpioInterruptStatus));
                R_RETURN(::gpioPadGetInterruptStatus(std::addressof(m_srv), reinterpret_cast<::GpioInterruptStatus *>(out.GetPointer())));
            }

            Result ClearInterruptStatus() {
                R_RETURN(::gpioPadClearInterruptStatus(std::addressof(m_srv)));
            }

            Result SetValue(gpio::GpioValue value) {
                R_RETURN(::gpioPadSetValue(std::addressof(m_srv), static_cast<::GpioValue>(static_cast<u32>(value))));
            }

            Result GetValue(ams::sf::Out<gpio::GpioValue> out) {
                static_assert(sizeof(gpio::GpioValue) == sizeof(::GpioValue));
                R_RETURN(::gpioPadGetValue(std::addressof(m_srv), reinterpret_cast<::GpioValue *>(out.GetPointer())));
            }

            Result BindInterrupt(ams::sf::OutCopyHandle out) {
                ::Event ev;
                R_TRY(::gpioPadBindInterrupt(std::addressof(m_srv), std::addressof(ev)));
                out.SetValue(ev.revent, true);
                R_SUCCEED();
            }

            Result UnbindInterrupt() {
                R_RETURN(::gpioPadUnbindInterrupt(std::addressof(m_srv)));
            }

            Result SetDebounceEnabled(bool enable) {
                R_RETURN(::gpioPadSetDebounceEnabled(std::addressof(m_srv), enable));
            }

            Result GetDebounceEnabled(ams::sf::Out<bool> out) {
                R_RETURN(::gpioPadGetDebounceEnabled(std::addressof(m_srv), out.GetPointer()));
            }

            Result SetDebounceTime(s32 ms) {
                R_RETURN(::gpioPadSetDebounceTime(std::addressof(m_srv), ms));
            }

            Result GetDebounceTime(ams::sf::Out<s32> out) {
                R_RETURN(::gpioPadGetDebounceTime(std::addressof(m_srv), out.GetPointer()));
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
    #endif

}
