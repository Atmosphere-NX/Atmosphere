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
            os::Event m_cancel_event;
            bool m_cancelled;
            os::Event m_connection_event;
            bool m_client_connected;
            bool m_server_connected;
            bool m_connected;
            os::SdkMutex m_connection_mutex;
        private:
            static void ClientThreadEntry(void *arg) { static_cast<HtcmiscImpl *>(arg)->ClientThread(); }
            static void ServerThreadEntry(void *arg) { static_cast<HtcmiscImpl *>(arg)->ServerThread(); }

            void ClientThread();
            void ServerThread();
        public:
            HtcmiscImpl(htclow::HtclowManager *htclow_manager);
            ~HtcmiscImpl();
        private:
            void SetClientConnectionEvent(bool en);
            void SetServerConnectionEvent(bool en);

            void UpdateConnectionEvent();
        public:
            void Cancel();
            /* TODO */
    };

}
