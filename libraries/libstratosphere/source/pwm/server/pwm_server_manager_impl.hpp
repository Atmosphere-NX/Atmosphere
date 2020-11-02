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
#include "pwm_server_channel_session_impl.hpp"

namespace ams::pwm::server {

    class ManagerImpl {
        private:
            lmem::HeapHandle heap_handle;
            ams::sf::ExpHeapMemoryResource session_memory_resource;
            typename ams::sf::ServiceObjectAllocator<pwm::sf::IChannelSession, ChannelSessionImpl> allocator;
            u8 heap_buffer[4_KB];
        public:
            ManagerImpl();

            ~ManagerImpl();
        public:
            /* Actual commands. */
            Result OpenSessionForDev(ams::sf::Out<std::shared_ptr<pwm::sf::IChannelSession>> out, int channel);
            Result OpenSession(ams::sf::Out<std::shared_ptr<pwm::sf::IChannelSession>> out, pwm::ChannelName channel_name);
            Result OpenSession2(ams::sf::Out<std::shared_ptr<pwm::sf::IChannelSession>> out, DeviceCode device_code);
    };
    static_assert(pwm::sf::IsIManager<ManagerImpl>);

}
