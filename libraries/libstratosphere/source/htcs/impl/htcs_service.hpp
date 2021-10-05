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
            Result CreateSocket(s32 *out_err, s32 *out_desc, bool enable_disconnection_emulation);
            Result DestroySocket(s32 desc);
            Result Connect(s32 *out_err, s32 desc, const SockAddrHtcs &address);
            Result Bind(s32 *out_err, s32 desc, const SockAddrHtcs &address);
            Result Listen(s32 *out_err, s32 desc, s32 backlog_count);
            Result Receive(s32 *out_err, s64 *out_size, char *buffer, s64 size, s32 desc, s32 flags);
            Result Send(s32 *out_err, s64 *out_size, const char *buffer, s64 size, s32 desc, s32 flags);
            Result Shutdown(s32 *out_err, s32 desc, s32 how);
            Result Fcntl(s32 *out_err, s32 *out_res, s32 desc, s32 command, s32 value);

            Result AcceptStart(u32 *out_task_id, os::NativeHandle *out_handle, s32 desc);
            Result AcceptResults(s32 *out_err, s32 *out_desc, SockAddrHtcs *out_address, u32 task_id, s32 desc);

            Result ReceiveSmallStart(u32 *out_task_id, os::NativeHandle *out_handle, s64 size, s32 desc, s32 flags);
            Result ReceiveSmallResults(s32 *out_err, s64 *out_size, char *buffer, s64 buffer_size, u32 task_id, s32 desc);

            Result SendSmallStart(u32 *out_task_id, os::NativeHandle *out_handle, s32 desc, s64 size, s32 flags);
            Result SendSmallContinue(s64 *out_size, const char *buffer, s64 buffer_size, u32 task_id, s32 desc);
            Result SendSmallResults(s32 *out_err, s64 *out_size, u32 task_id, s32 desc);

            Result SendStart(u32 *out_task_id, os::NativeHandle *out_handle, s32 desc, s64 size, s32 flags);
            Result SendContinue(s64 *out_size, const char *buffer, s64 buffer_size, u32 task_id, s32 desc);
            Result SendResults(s32 *out_err, s64 *out_size, u32 task_id, s32 desc);

            Result ReceiveStart(u32 *out_task_id, os::NativeHandle *out_handle, s64 size, s32 desc, s32 flags);
            Result ReceiveResults(s32 *out_err, s64 *out_size, char *buffer, s64 buffer_size, u32 task_id, s32 desc);

            Result SelectStart(u32 *out_task_id, os::NativeHandle *out_handle, Span<const int> read_handles, Span<const int> write_handles, Span<const int> exception_handles, s64 tv_sec, s64 tv_usec);
            Result SelectEnd(s32 *out_err, bool *out_empty, Span<int> read_handles, Span<int> write_handles, Span<int> exception_handles, u32 task_id);
        private:
            void WaitTask(u32 task_id);
    };

}
