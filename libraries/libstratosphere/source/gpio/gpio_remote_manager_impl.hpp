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
#include "gpio_remote_pad_session_impl.hpp"

namespace ams::gpio {

    class RemoteManagerImpl {
        public:
            RemoteManagerImpl() { /* ... */ }

            ~RemoteManagerImpl() { /* ... */ }
        public:
            /* Actual commands. */
            Result OpenSessionForDev(ams::sf::Out<std::shared_ptr<gpio::sf::IPadSession>> out, s32 pad_descriptor) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result OpenSession(ams::sf::Out<std::shared_ptr<gpio::sf::IPadSession>> out, gpio::GpioPadName pad_name) {
                ::GpioPadSession p;
                R_TRY(::gpioOpenSession(std::addressof(p), static_cast<::GpioPadName>(static_cast<u32>(pad_name))));

                out.SetValue(ams::sf::MakeShared<gpio::sf::IPadSession, RemotePadSessionImpl>(p));
                return ResultSuccess();
            }

            Result OpenSessionForTest(ams::sf::Out<std::shared_ptr<gpio::sf::IPadSession>> out, gpio::GpioPadName pad_name) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result IsWakeEventActive(ams::sf::Out<bool> out, gpio::GpioPadName pad_name) {
                return ::gpioIsWakeEventActive2(out.GetPointer(), static_cast<::GpioPadName>(static_cast<u32>(pad_name)));
            }

            Result GetWakeEventActiveFlagSet(ams::sf::Out<gpio::WakeBitFlag> out) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result SetWakeEventActiveFlagSetForDebug(gpio::GpioPadName pad_name, bool is_enabled) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result SetWakePinDebugMode(s32 mode) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result OpenSession2(ams::sf::Out<std::shared_ptr<gpio::sf::IPadSession>> out, DeviceCode device_code, ddsf::AccessMode access_mode) {
                ::GpioPadSession p;
                R_TRY(::gpioOpenSession2(std::addressof(p), device_code.GetInternalValue(), access_mode));

                out.SetValue(ams::sf::MakeShared<gpio::sf::IPadSession, RemotePadSessionImpl>(p));
                return ResultSuccess();
            }

            Result IsWakeEventActive2(ams::sf::Out<bool> out, DeviceCode device_code) {
                return ::gpioIsWakeEventActive2(out.GetPointer(), device_code.GetInternalValue());
            }

            Result SetWakeEventActiveFlagSetForDebug2(DeviceCode device_code, bool is_enabled) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result SetRetryValues(u32 arg0, u32 arg1) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

    };
    static_assert(gpio::sf::IsIManager<RemoteManagerImpl>);

}
