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
#include "../../htclow/htclow_manager.hpp"
#include "../../htc/server/driver/htc_htclow_driver.hpp"
#include "../../htc/server/driver/htc_driver_manager.hpp"
#include "../../htc/server/rpc/htc_rpc_client.hpp"
#include "rpc/htcs_data_channel_manager.hpp"
#include "htcs_service.hpp"
#include "htcs_monitor.hpp"

namespace ams::htcs::impl {

    class HtcsManagerImpl {
        private:
            mem::StandardAllocator *m_allocator;
            htc::server::driver::HtclowDriver m_driver;
            htc::server::driver::DriverManager m_driver_manager;
            htc::server::rpc::RpcClient m_rpc_client;
            rpc::DataChannelManager m_data_channel_manager;
            HtcsService m_service;
            HtcsMonitor m_monitor;
        public:
            HtcsManagerImpl(mem::StandardAllocator *allocator, htclow::HtclowManager *htclow_manager);
            ~HtcsManagerImpl();
        public:
            os::EventType *GetServiceAvailabilityEvent();

            bool IsServiceAvailable();
    };

}
