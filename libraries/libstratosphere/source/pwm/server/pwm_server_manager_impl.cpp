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
#include <stratosphere.hpp>
#include "pwm_server_manager_impl.hpp"

namespace ams::pwm::server {

    ManagerImpl::ManagerImpl() {
        m_heap_handle = lmem::CreateExpHeap(m_heap_buffer, sizeof(m_heap_buffer), lmem::CreateOption_None);
        m_allocator.Attach(m_heap_handle);
    }

    ManagerImpl::~ManagerImpl() {
        lmem::DestroyExpHeap(m_heap_handle);
    }

    Result ManagerImpl::OpenSessionForDev(ams::sf::Out<ams::sf::SharedPointer<pwm::sf::IChannelSession>> out, int channel) {
        /* TODO */
        AMS_UNUSED(out, channel);
        AMS_ABORT();
    }

    Result ManagerImpl::OpenSession(ams::sf::Out<ams::sf::SharedPointer<pwm::sf::IChannelSession>> out, pwm::ChannelName channel_name) {
        return this->OpenSession2(out, ConvertToDeviceCode(channel_name));
    }

    Result ManagerImpl::OpenSession2(ams::sf::Out<ams::sf::SharedPointer<pwm::sf::IChannelSession>> out, DeviceCode device_code) {
        /* Allocate a session. */
        auto session = Factory::CreateSharedEmplaced<pwm::sf::IChannelSession, ChannelSessionImpl>(std::addressof(m_allocator), this);

        /* Open the session. */
        R_TRY(session.GetImpl().OpenSession(device_code));

        /* We succeeded. */
        *out = std::move(session);
        return ResultSuccess();
    }

}
