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
#include "driver/htc_htclow_driver.hpp"
#include "driver/htc_driver_manager.hpp"
#include "rpc/htc_rpc_client.hpp"
#include "rpc/htc_htcmisc_rpc_server.hpp"

namespace ams::htc::server {

    class HtcmiscImpl {
        private:
            driver::HtclowDriver m_htclow_driver;
            driver::DriverManager m_driver_manager;
            rpc::RpcClient m_rpc_client;
            rpc::HtcmiscRpcServer m_rpc_server;
            os::ThreadType m_client_thread;
            os::ThreadType m_server_thread;
            os::Event m_event_61200;
            u8 m_61228;
            os::Event m_event_61230;
            bool m_client_connected;
            bool m_server_connected;
            u8 m_6125A;
            os::SdkMutex m_connection_mutex;
        public:
            HtcmiscImpl(htclow::HtclowManager *htclow_manager);
        public:
            /* TODO */
    };

}
