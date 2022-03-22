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
#include "pwm_server_channel_session_impl.hpp"

namespace ams::pwm::server {

    class ManagerImpl {
        private:
            using Allocator = ams::sf::ExpHeapAllocator;
            using Factory   = ams::sf::ObjectFactory<Allocator::Policy>;
        private:
            lmem::HeapHandle m_heap_handle;
            Allocator m_allocator;
            u8 m_heap_buffer[4_KB];
        public:
            ManagerImpl();

            ~ManagerImpl();
        public:
            /* Actual commands. */
            Result OpenSessionForDev(ams::sf::Out<ams::sf::SharedPointer<pwm::sf::IChannelSession>> out, int channel);
            Result OpenSession(ams::sf::Out<ams::sf::SharedPointer<pwm::sf::IChannelSession>> out, pwm::ChannelName channel_name);
            Result OpenSession2(ams::sf::Out<ams::sf::SharedPointer<pwm::sf::IChannelSession>> out, DeviceCode device_code);
    };
    static_assert(pwm::sf::IsIManager<ManagerImpl>);

}
