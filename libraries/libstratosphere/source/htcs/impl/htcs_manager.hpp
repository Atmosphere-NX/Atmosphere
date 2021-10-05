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
#include "../../htclow/htclow_manager.hpp"

namespace ams::htcs::impl {

    class HtcsManagerImpl;

    class HtcsManager {
        private:
            mem::StandardAllocator *m_allocator;
            HtcsManagerImpl *m_impl;
        public:
            HtcsManager(mem::StandardAllocator *allocator, htclow::HtclowManager *htclow_manager);
            ~HtcsManager();
        public:
            os::EventType *GetServiceAvailabilityEvent();

            bool IsServiceAvailable();
        public:
            void Socket(s32 *out_err, s32 *out_desc, bool enable_disconnection_emulation);
            void Close(s32 *out_err, s32 *out_res, s32 desc);
            void Connect(s32 *out_err, s32 *out_res, const SockAddrHtcs &address, s32 desc);
            void Bind(s32 *out_err, s32 *out_res, const SockAddrHtcs &address, s32 desc);
            void Listen(s32 *out_err, s32 *out_res, s32 backlog_count, s32 desc);
            void Recv(s32 *out_err, s64 *out_size, char *buffer, size_t size, s32 flags, s32 desc);
            void Send(s32 *out_err, s64 *out_size, const char *buffer, size_t size, s32 flags, s32 desc);
            void Shutdown(s32 *out_err, s32 *out_res, s32 how, s32 desc);
            void Fcntl(s32 *out_err, s32 *out_res, s32 command, s32 value, s32 desc);

            Result AcceptStart(u32 *out_task_id, os::NativeHandle *out_handle, s32 desc);
            void AcceptResults(s32 *out_err, s32 *out_desc, SockAddrHtcs *out_address, u32 task_id, s32 desc);

            Result RecvStart(u32 *out_task_id, os::NativeHandle *out_handle, s64 size, s32 desc, s32 flags);
            void RecvResults(s32 *out_err, s64 *out_size, char *buffer, s64 buffer_size, u32 task_id, s32 desc);

            Result SendStart(u32 *out_task_id, os::NativeHandle *out_handle, const char *buffer, s64 size, s32 desc, s32 flags);
            Result SendLargeStart(u32 *out_task_id, os::NativeHandle *out_handle, const char **buffers, const s64 *sizes, s32 count, s32 desc, s32 flags);
            void SendResults(s32 *out_err, s64 *out_size, u32 task_id, s32 desc);

            Result StartSend(u32 *out_task_id, os::NativeHandle *out_handle, s32 desc, s64 size, s32 flags);
            Result ContinueSend(s64 *out_size, const char *buffer, s64 buffer_size, u32 task_id, s32 desc);
            void EndSend(s32 *out_err, s64 *out_size, u32 task_id, s32 desc);

            Result StartRecv(u32 *out_task_id, os::NativeHandle *out_handle, s64 size, s32 desc, s32 flags);
            void EndRecv(s32 *out_err, s64 *out_size, char *buffer, s64 buffer_size, u32 task_id, s32 desc);

            Result StartSelect(u32 *out_task_id, os::NativeHandle *out_handle, Span<const int> read_handles, Span<const int> write_handles, Span<const int> exception_handles, s64 tv_sec, s64 tv_usec);
            Result EndSelect(s32 *out_err, s32 *out_count, Span<int> read_handles, Span<int> write_handles, Span<int> exception_handles, u32 task_id);
    };

}
