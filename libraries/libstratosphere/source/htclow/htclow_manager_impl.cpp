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
#include "htclow_manager_impl.hpp"

namespace ams::htclow {

    HtclowManagerImpl::HtclowManagerImpl(mem::StandardAllocator *allocator)
        : m_packet_factory(allocator), m_driver_manager(), m_mux(std::addressof(m_packet_factory), std::addressof(m_ctrl_state_machine)),
          m_ctrl_packet_factory(allocator), m_ctrl_state_machine(), m_ctrl_service(std::addressof(m_ctrl_packet_factory), std::addressof(m_ctrl_state_machine), std::addressof(m_mux)),
          m_worker(allocator, std::addressof(m_mux), std::addressof(m_ctrl_service)),
          m_listener(allocator, std::addressof(m_mux), std::addressof(m_ctrl_service), std::addressof(m_worker)),
          m_is_driver_open(false)
    {
        /* ... */
    }

    HtclowManagerImpl::~HtclowManagerImpl() {
        /* ... */
    }

    Result HtclowManagerImpl::OpenDriver(impl::DriverType driver_type) {
        /* Set the driver type. */
        m_ctrl_service.SetDriverType(driver_type);

        /* Ensure that we don't end up in an invalid state. */
        auto drv_guard = SCOPE_GUARD { m_ctrl_service.SetDriverType(impl::DriverType::Unknown); };

        /* Try to open the driver. */
        R_TRY(m_driver_manager.OpenDriver(driver_type));

        /* Start the listener. */
        m_listener.Start(m_driver_manager.GetCurrentDriver());

        /* Note the driver as open. */
        m_is_driver_open = true;

        drv_guard.Cancel();
        return ResultSuccess();
    }

    void HtclowManagerImpl::CloseDriver() {
        AMS_ABORT("HtclowManagerImpl::CloseDriver");
    }

    void HtclowManagerImpl::Disconnect() {
        AMS_ABORT("HtclowManagerImpl::Disconnect");
    }

}
