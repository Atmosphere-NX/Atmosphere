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

    Result HtcsManagerImpl::CreateSocket(s32 *out_err, s32 *out_desc, bool enable_disconnection_emulation) {
        return m_service.CreateSocket(out_err, out_desc, enable_disconnection_emulation);
    }

    Result HtcsManagerImpl::DestroySocket(s32 desc) {
        return m_service.DestroySocket(desc);
    }

    Result HtcsManagerImpl::Connect(s32 *out_err, s32 desc, const SockAddrHtcs &address) {
        return m_service.Connect(out_err, desc, address);
    }

    Result HtcsManagerImpl::Bind(s32 *out_err, s32 desc, const SockAddrHtcs &address) {
        return m_service.Bind(out_err, desc, address);
    }

    Result HtcsManagerImpl::Listen(s32 *out_err, s32 desc, s32 backlog_count) {
        return m_service.Listen(out_err, desc, backlog_count);
    }

    Result HtcsManagerImpl::Receive(s32 *out_err, s64 *out_size, char *buffer, size_t size, s32 desc, s32 flags) {
        return m_service.Receive(out_err, out_size, buffer, size, desc, flags);
    }

    Result HtcsManagerImpl::Send(s32 *out_err, s64 *out_size, const char *buffer, size_t size, s32 desc, s32 flags) {
        return m_service.Send(out_err, out_size, buffer, size, desc, flags);
    }

    Result HtcsManagerImpl::Shutdown(s32 *out_err, s32 desc, s32 how) {
        return m_service.Shutdown(out_err, desc, how);
    }

    Result HtcsManagerImpl::Fcntl(s32 *out_err, s32 *out_res, s32 desc, s32 command, s32 value) {
        return m_service.Fcntl(out_err, out_res, desc, command, value);
    }

    Result HtcsManagerImpl::AcceptStart(u32 *out_task_id, Handle *out_handle, s32 desc) {
        return m_service.AcceptStart(out_task_id, out_handle, desc);
    }

    Result HtcsManagerImpl::AcceptResults(s32 *out_err, s32 *out_desc, SockAddrHtcs *out_address, u32 task_id, s32 desc) {
        return m_service.AcceptResults(out_err, out_desc, out_address, task_id, desc);
    }

    Result HtcsManagerImpl::RecvStart(u32 *out_task_id, Handle *out_handle, s64 size, s32 desc, s32 flags) {
        return m_service.ReceiveSmallStart(out_task_id, out_handle, size, desc, flags);
    }

    Result HtcsManagerImpl::RecvResults(s32 *out_err, s64 *out_size, char *buffer, s64 buffer_size, u32 task_id, s32 desc) {
        return m_service.ReceiveSmallResults(out_err, out_size, buffer, buffer_size, task_id, desc);
    }

    Result HtcsManagerImpl::SendStart(u32 *out_task_id, Handle *out_handle, const char *buffer, s64 size, s32 desc, s32 flags) {
        /* Start the send. */
        u32 task_id;
        Handle handle;
        R_TRY(m_service.SendSmallStart(std::addressof(task_id), std::addressof(handle), desc, size, flags));

        /* Continue the send. */
        s64 continue_size;
        const Result result = m_service.SendSmallContinue(std::addressof(continue_size), buffer, size, task_id, desc);
        if (R_SUCCEEDED(result) || htcs::ResultCompleted::Includes(result) || htc::ResultTaskQueueNotAvailable::Includes(result)) {
            *out_task_id = task_id;
            *out_handle  = handle;
        } else {
            os::SystemEventType event;
            os::AttachReadableHandleToSystemEvent(std::addressof(event), handle, true, os::EventClearMode_ManualClear);

            s32 err;
            s64 rsize;
            m_service.SendSmallResults(std::addressof(err), std::addressof(rsize), task_id, desc);

            os::DestroySystemEvent(std::addressof(event));

            return result;
        }

        return ResultSuccess();
    }

    Result HtcsManagerImpl::SendLargeStart(u32 *out_task_id, Handle *out_handle, const char **buffers, const s64 *sizes, s32 count, s32 desc, s32 flags) {
        /* NOTE: Nintendo aborts here, too. */
        AMS_ABORT("HtcsManagerImpl::SendLargeStart is not implemented");
    }

    Result HtcsManagerImpl::SendResults(s32 *out_err, s64 *out_size, u32 task_id, s32 desc) {
        return m_service.SendSmallResults(out_err, out_size, task_id, desc);
    }

    Result HtcsManagerImpl::StartSend(u32 *out_task_id, Handle *out_handle, s32 desc, s64 size, s32 flags) {
        return m_service.SendStart(out_task_id, out_handle, desc, size, flags);
    }

    Result HtcsManagerImpl::ContinueSend(s64 *out_size, const char *buffer, s64 buffer_size, u32 task_id, s32 desc) {
        return m_service.SendContinue(out_size, buffer, buffer_size, task_id, desc);
    }

    Result HtcsManagerImpl::EndSend(s32 *out_err, s64 *out_size, u32 task_id, s32 desc) {
        return m_service.SendResults(out_err, out_size, task_id, desc);
    }

    Result HtcsManagerImpl::StartRecv(u32 *out_task_id, Handle *out_handle, s64 size, s32 desc, s32 flags) {
        return m_service.ReceiveStart(out_task_id, out_handle, size, desc, flags);
    }

    Result HtcsManagerImpl::EndRecv(s32 *out_err, s64 *out_size, char *buffer, s64 buffer_size, u32 task_id, s32 desc) {
        return m_service.ReceiveResults(out_err, out_size, buffer, buffer_size, task_id, desc);
    }

    Result HtcsManagerImpl::StartSelect(u32 *out_task_id, Handle *out_handle, Span<const int> read_handles, Span<const int> write_handles, Span<const int> exception_handles, s64 tv_sec, s64 tv_usec) {
        /* Start the select. */
        u32 task_id;
        Handle handle;
        const Result result = m_service.SelectStart(std::addressof(task_id), std::addressof(handle), read_handles, write_handles, exception_handles, tv_sec, tv_usec);

        /* Ensure our state ends up clean. */
        if (htcs::ResultCancelled::Includes(result)) {
            os::SystemEventType event;
            os::AttachReadableHandleToSystemEvent(std::addressof(event), handle, true, os::EventClearMode_ManualClear);

            s32 err;
            bool empty;
            m_service.SelectEnd(std::addressof(err), std::addressof(empty), Span<int>{}, Span<int>{}, Span<int>{}, task_id);

            os::DestroySystemEvent(std::addressof(event));
        } else {
            *out_task_id = task_id;
            *out_handle  = handle;
        }

        return result;
    }

    Result HtcsManagerImpl::EndSelect(s32 *out_err, bool *out_empty, Span<int> read_handles, Span<int> write_handles, Span<int> exception_handles, u32 task_id) {
        return m_service.SelectEnd(out_err, out_empty, read_handles, write_handles, exception_handles, task_id);
    }

}
