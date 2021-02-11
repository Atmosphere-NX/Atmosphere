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
#include <stratosphere.hpp>
#include "htcs_manager.hpp"
#include "htcs_manager_impl.hpp"

namespace ams::htcs::impl {

    HtcsManagerImpl::HtcsManagerImpl(mem::StandardAllocator *allocator, htclow::HtclowManager *htclow_manager)
        : m_allocator(allocator),
          m_driver(htclow_manager, htclow::ModuleId::Htcs),
          m_driver_manager(std::addressof(m_driver)),
          m_rpc_client(m_allocator, std::addressof(m_driver), HtcsClientChannelId),
          m_data_channel_manager(std::addressof(m_rpc_client), htclow_manager),
          m_service(m_allocator, m_driver_manager.GetDriver(), std::addressof(m_rpc_client), std::addressof(m_data_channel_manager)),
          m_monitor(m_allocator, m_driver_manager.GetDriver(), std::addressof(m_rpc_client), std::addressof(m_service))
    {
        /* Start the monitor. */
        m_monitor.Start();
    }

    HtcsManagerImpl::~HtcsManagerImpl() {
        /* Cancel our monitor. */
        m_monitor.Cancel();
        m_monitor.Wait();
    }

    os::EventType *HtcsManagerImpl::GetServiceAvailabilityEvent() {
        return m_monitor.GetServiceAvailabilityEvent();
    }

    bool HtcsManagerImpl::IsServiceAvailable() {
        return m_monitor.IsServiceAvailable();
    }

}
