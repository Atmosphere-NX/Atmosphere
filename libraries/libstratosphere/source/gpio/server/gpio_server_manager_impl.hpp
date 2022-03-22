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
#include "gpio_server_pad_session_impl.hpp"

namespace ams::gpio::server {

    class ManagerImpl : public ams::sf::ISharedObject {
        private:
            using Allocator = ams::sf::ExpHeapAllocator;
            using Factory   = ams::sf::ObjectFactory<Allocator::Policy>;
        private:
            lmem::HeapHandle m_heap_handle;
            Allocator m_pad_allocator;
            u8 m_heap_buffer[12_KB];
        public:
            ManagerImpl();

            ~ManagerImpl();
        public:
            /* Actual commands. */
            Result OpenSessionForDev(ams::sf::Out<ams::sf::SharedPointer<gpio::sf::IPadSession>> out, s32 pad_descriptor);
            Result OpenSession(ams::sf::Out<ams::sf::SharedPointer<gpio::sf::IPadSession>> out, gpio::GpioPadName pad_name);
            Result OpenSessionForTest(ams::sf::Out<ams::sf::SharedPointer<gpio::sf::IPadSession>> out, gpio::GpioPadName pad_name);
            Result IsWakeEventActive(ams::sf::Out<bool> out, gpio::GpioPadName pad_name);
            Result GetWakeEventActiveFlagSet(ams::sf::Out<gpio::WakeBitFlag> out);
            Result SetWakeEventActiveFlagSetForDebug(gpio::GpioPadName pad_name, bool is_enabled);
            Result SetWakePinDebugMode(s32 mode);
            Result OpenSession2(ams::sf::Out<ams::sf::SharedPointer<gpio::sf::IPadSession>> out, DeviceCode device_code, ddsf::AccessMode access_mode);
            Result IsWakeEventActive2(ams::sf::Out<bool> out, DeviceCode device_code);
            Result SetWakeEventActiveFlagSetForDebug2(DeviceCode device_code, bool is_enabled);
            Result SetRetryValues(u32 arg0, u32 arg1);

    };
    static_assert(gpio::sf::IsIManager<ManagerImpl>);

}
