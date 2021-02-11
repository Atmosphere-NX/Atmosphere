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
#include "../../htc/server/driver/htc_i_driver.hpp"
#include "../../htc/server/rpc/htc_rpc_client.hpp"
#include "rpc/htcs_data_channel_manager.hpp"

namespace ams::htcs::impl {

    class HtcsService {
        private:
            mem::StandardAllocator *m_allocator;
            htc::server::driver::IDriver *m_driver;
            htc::server::rpc::RpcClient *m_rpc_client;
            rpc::DataChannelManager *m_data_channel_manager;
        public:
            HtcsService(mem::StandardAllocator *allocator, htc::server::driver::IDriver *drv, htc::server::rpc::RpcClient *rc, rpc::DataChannelManager *dcm)
                : m_allocator(allocator), m_driver(drv), m_rpc_client(rc), m_data_channel_manager(dcm) { /* ... */ }
        public:
            /* TODO */
    };

}
