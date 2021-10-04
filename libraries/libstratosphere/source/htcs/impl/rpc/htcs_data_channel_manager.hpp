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
#include "../../../htclow/htclow_manager.hpp"
#include "../../../htc/server/rpc/htc_rpc_client.hpp"

namespace ams::htcs::impl::rpc {

    class DataChannelManager {
        private:
            htc::server::rpc::RpcClient* m_rpc_client;
            htclow::HtclowManager *m_htclow_manager;
            htclow::Module m_module;
        public:
            DataChannelManager(htc::server::rpc::RpcClient *client, htclow::HtclowManager *htclow_manager) : m_rpc_client(client), m_htclow_manager(htclow_manager), m_module(htclow::ModuleId::Htcs) { /* ... */ }
        public:
            Result Receive(void *buffer, s64 buffer_size, u32 task_id);
            Result Send(const void *buffer, s64 buffer_size, u32 task_id);
    };

}
