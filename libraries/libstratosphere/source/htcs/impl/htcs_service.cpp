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
#include <stratosphere.hpp>
#include "htcs_service.hpp"
#include "rpc/htcs_rpc_tasks.hpp"
#include "htcs_util.hpp"

namespace ams::htcs::impl {

    void HtcsService::WaitTask(u32 task_id) {
        return m_rpc_client->Wait(task_id);
    }

    Result HtcsService::CreateSocket(s32 *out_err, s32 *out_desc, bool enable_disconnection_emulation) {
        /* Set disconnection emulation enabled. */
        m_driver->SetDisconnectionEmulationEnabled(enable_disconnection_emulation);

        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client->Begin<rpc::SocketTask>(std::addressof(task_id)));

        /* Wait for the task to complete. */
        this->WaitTask(task_id);

        /* Finish the task. */
        htcs::SocketError err;
        s32 desc;
        R_TRY(m_rpc_client->End<rpc::SocketTask>(task_id, std::addressof(err), std::addressof(desc)));

        /* Set output. */
        *out_err  = err;
        *out_desc = desc;
        return ResultSuccess();
    }

    Result HtcsService::DestroySocket(s32 desc) {
        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client->Begin<rpc::CloseTask>(std::addressof(task_id), desc));

        /* Wait for the task to complete. */
        this->WaitTask(task_id);

        /* Cancel the socket. */
        m_rpc_client->CancelBySocket(desc);

        /* Finish the task. */
        htcs::SocketError err;
        R_TRY(m_rpc_client->End<rpc::CloseTask>(task_id, std::addressof(err)));

        return ResultSuccess();
    }

    Result HtcsService::Connect(s32 *out_err, s32 desc, const SockAddrHtcs &address) {
        /* Validate the address. */
        R_UNLESS(address.family == 0,            htcs::ResultInvalidArgument());
        R_UNLESS(IsValidName(address.peer_name), htcs::ResultInvalidArgument());
        R_UNLESS(IsValidName(address.port_name), htcs::ResultInvalidArgument());

        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client->Begin<rpc::ConnectTask>(std::addressof(task_id), desc, address.peer_name, address.port_name));

        /* Wait for the task to complete. */
        this->WaitTask(task_id);

        /* Finish the task. */
        htcs::SocketError err;
        R_TRY(m_rpc_client->End<rpc::ConnectTask>(task_id, std::addressof(err)));

        /* Set output. */
        *out_err = err;
        return ResultSuccess();
    }

    Result HtcsService::Bind(s32 *out_err, s32 desc, const SockAddrHtcs &address) {
        /* Validate the address. */
        R_UNLESS(address.family == 0,            htcs::ResultInvalidArgument());
        R_UNLESS(IsValidName(address.peer_name), htcs::ResultInvalidArgument());
        R_UNLESS(IsValidName(address.port_name), htcs::ResultInvalidArgument());

        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client->Begin<rpc::BindTask>(std::addressof(task_id), desc, address.peer_name, address.port_name));

        /* Wait for the task to complete. */
        this->WaitTask(task_id);

        /* Finish the task. */
        htcs::SocketError err;
        R_TRY(m_rpc_client->End<rpc::BindTask>(task_id, std::addressof(err)));

        /* Set output. */
        *out_err = err;
        return ResultSuccess();
    }

    Result HtcsService::Listen(s32 *out_err, s32 desc, s32 backlog_count) {
        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client->Begin<rpc::ListenTask>(std::addressof(task_id), desc, backlog_count));

        /* Wait for the task to complete. */
        this->WaitTask(task_id);

        /* Finish the task. */
        htcs::SocketError err;
        R_TRY(m_rpc_client->End<rpc::ListenTask>(task_id, std::addressof(err)));

        /* Set output. */
        *out_err = err;
        return ResultSuccess();
    }

    Result HtcsService::Receive(s32 *out_err, s64 *out_size, char *buffer, s64 size, s32 desc, s32 flags) {
        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client->Begin<rpc::ReceiveTask>(std::addressof(task_id), desc, size, static_cast<htcs::MessageFlag>(flags)));

        /* Wait for the task to complete. */
        this->WaitTask(task_id);

        /* Finish the task. */
        R_TRY(this->ReceiveResults(out_err, out_size, buffer, size, task_id, desc));

        return ResultSuccess();
    }

    Result HtcsService::Send(s32 *out_err, s64 *out_size, const char *buffer, s64 size, s32 desc, s32 flags) {
        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client->Begin<rpc::SendTask>(std::addressof(task_id), desc, size, static_cast<htcs::MessageFlag>(flags)));

        /* Send the data. */
        s64 cont_size;
        const Result result = this->SendContinue(std::addressof(cont_size), buffer, size, task_id, desc);
        if (R_FAILED(result)) {
            return this->SendResults(out_err, out_size, task_id, desc);
        }

        /* Wait for the task to complete. */
        this->WaitTask(task_id);

        /* Finish the task. */
        R_TRY(this->SendResults(out_err, out_size, task_id, desc));

        return ResultSuccess();
    }

    Result HtcsService::Shutdown(s32 *out_err, s32 desc, s32 how) {
        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client->Begin<rpc::ShutdownTask>(std::addressof(task_id), desc, static_cast<htcs::ShutdownType>(how)));

        /* Wait for the task to complete. */
        this->WaitTask(task_id);

        /* Finish the task. */
        htcs::SocketError err;
        R_TRY(m_rpc_client->End<rpc::ShutdownTask>(task_id, std::addressof(err)));

        /* Set output. */
        *out_err = err;
        return ResultSuccess();
    }

    Result HtcsService::Fcntl(s32 *out_err, s32 *out_res, s32 desc, s32 command, s32 value) {
        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client->Begin<rpc::FcntlTask>(std::addressof(task_id), desc, command, value));

        /* Wait for the task to complete. */
        this->WaitTask(task_id);

        /* Finish the task. */
        htcs::SocketError err;
        s32 res;
        R_TRY(m_rpc_client->End<rpc::FcntlTask>(task_id, std::addressof(err), std::addressof(res)));

        /* Set output. */
        *out_err = err;
        *out_res = res;
        return ResultSuccess();
    }

    Result HtcsService::AcceptStart(u32 *out_task_id, os::NativeHandle *out_handle, s32 desc) {
        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client->Begin<rpc::AcceptTask>(std::addressof(task_id), desc));

        /* Detach the task. */
        *out_task_id = task_id;
        *out_handle  = m_rpc_client->DetachReadableHandle(task_id);

        return ResultSuccess();
    }

    Result HtcsService::AcceptResults(s32 *out_err, s32 *out_desc, SockAddrHtcs *out_address, u32 task_id, s32 desc) {
        AMS_UNUSED(out_address);

        /* Finish the task. */
        htcs::SocketError err;
        s32 ret_desc;
        R_TRY(m_rpc_client->End<rpc::AcceptTask>(task_id, std::addressof(err), std::addressof(ret_desc), desc));

        /* Set output. */
        *out_err  = err;
        *out_desc = ret_desc;
        return ResultSuccess();
    }

    Result HtcsService::ReceiveSmallStart(u32 *out_task_id, os::NativeHandle *out_handle, s64 size, s32 desc, s32 flags) {
        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client->Begin<rpc::ReceiveSmallTask>(std::addressof(task_id), desc, size, static_cast<htcs::MessageFlag>(flags)));

        /* Detach the task. */
        *out_task_id = task_id;
        *out_handle  = m_rpc_client->DetachReadableHandle(task_id);

        return ResultSuccess();
    }

    Result HtcsService::ReceiveSmallResults(s32 *out_err, s64 *out_size, char *buffer, s64 buffer_size, u32 task_id, s32 desc) {
        AMS_UNUSED(desc);

        /* Continue the task. */
        m_rpc_client->ReceiveContinue<rpc::ReceiveSmallTask>(task_id, buffer, buffer_size);

        /* Finish the task. */
        htcs::SocketError err;
        R_TRY(m_rpc_client->End<rpc::ReceiveSmallTask>(task_id, std::addressof(err), out_size));

        /* Set output. */
        *out_err = err;
        return ResultSuccess();
    }

    Result HtcsService::SendSmallStart(u32 *out_task_id, os::NativeHandle *out_handle, s32 desc, s64 size, s32 flags) {
        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client->Begin<rpc::SendSmallTask>(std::addressof(task_id), desc, size, static_cast<htcs::MessageFlag>(flags)));

        /* Detach the task. */
        *out_task_id = task_id;
        *out_handle  = m_rpc_client->DetachReadableHandle(task_id);

        return ResultSuccess();
    }

    Result HtcsService::SendSmallContinue(s64 *out_size, const char *buffer, s64 buffer_size, u32 task_id, s32 desc) {
        /* Verify the task. */
        R_TRY(m_rpc_client->VerifyTaskIdWithHandle<rpc::SendSmallTask>(task_id, desc));

        /* Continue the task. */
        R_TRY(m_rpc_client->SendContinue<rpc::SendSmallTask>(task_id, buffer, buffer_size));

        /* Set output. */
        *out_size = buffer_size;
        return ResultSuccess();
    }

    Result HtcsService::SendSmallResults(s32 *out_err, s64 *out_size, u32 task_id, s32 desc) {
        AMS_UNUSED(desc);

        /* Finish the task. */
        htcs::SocketError err;
        R_TRY(m_rpc_client->End<rpc::SendSmallTask>(task_id, std::addressof(err), out_size));

        /* Set output. */
        *out_err = err;
        return ResultSuccess();
    }

    Result HtcsService::SendStart(u32 *out_task_id, os::NativeHandle *out_handle, s32 desc, s64 size, s32 flags) {
        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client->Begin<rpc::SendTask>(std::addressof(task_id), desc, size, static_cast<htcs::MessageFlag>(flags)));

        /* Detach the task. */
        *out_task_id = task_id;
        *out_handle  = m_rpc_client->DetachReadableHandle(task_id);

        return ResultSuccess();
    }

    Result HtcsService::SendContinue(s64 *out_size, const char *buffer, s64 buffer_size, u32 task_id, s32 desc) {
        /* Verify the task. */
        R_TRY(m_rpc_client->VerifyTaskIdWithHandle<rpc::SendTask>(task_id, desc));

        /* Wait for the task to notify. */
        m_rpc_client->WaitNotification<rpc::SendTask>(task_id);

        /* Check the task status. */
        R_UNLESS(!m_rpc_client->IsCompleted<rpc::SendTask>(task_id), htcs::ResultCompleted());
        R_UNLESS(!m_rpc_client->IsCancelled<rpc::SendTask>(task_id), htcs::ResultCancelled());

        /* Send the data. */
        if (buffer_size > 0) {
            R_TRY(m_data_channel_manager->Send(buffer, buffer_size, task_id));
        }

        /* Set output. */
        *out_size = buffer_size;
        return ResultSuccess();
    }

    Result HtcsService::SendResults(s32 *out_err, s64 *out_size, u32 task_id, s32 desc) {
        /* Verify the task. */
        R_TRY(m_rpc_client->VerifyTaskIdWithHandle<rpc::SendTask>(task_id, desc));

        /* Finish the task. */
        htcs::SocketError err;
        R_TRY(m_rpc_client->End<rpc::SendTask>(task_id, std::addressof(err), out_size));

        /* Set output. */
        *out_err = err;
        return ResultSuccess();
    }

    Result HtcsService::ReceiveStart(u32 *out_task_id, os::NativeHandle *out_handle, s64 size, s32 desc, s32 flags) {
        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client->Begin<rpc::ReceiveTask>(std::addressof(task_id), desc, size, static_cast<htcs::MessageFlag>(flags)));

        /* Detach the task. */
        *out_task_id = task_id;
        *out_handle  = m_rpc_client->DetachReadableHandle(task_id);

        return ResultSuccess();
    }

    Result HtcsService::ReceiveResults(s32 *out_err, s64 *out_size, char *buffer, s64 buffer_size, u32 task_id, s32 desc) {
        /* Verify the task. */
        R_TRY(m_rpc_client->VerifyTaskIdWithHandle<rpc::ReceiveTask>(task_id, desc));

        /* Get the result. */
        htcs::SocketError err;
        s64 recv_size;
        const Result result = m_rpc_client->GetResult<rpc::ReceiveTask>(task_id, std::addressof(err), std::addressof(recv_size));
        if (R_FAILED(result) || err != HTCS_ENONE) {
            /* Finish the task. */
            R_TRY(m_rpc_client->End<rpc::ReceiveTask>(task_id, std::addressof(err), out_size));

            /* Set output. */
            *out_err = err;
            return ResultSuccess();
        }

        /* Check the size. */
        R_UNLESS(recv_size <= buffer_size, htcs::ResultInvalidArgument());

        /* Perform remaining processing. */
        if (recv_size > 0) {
            /* Receive data. */
            const Result recv_result = m_data_channel_manager->Receive(buffer, recv_size, task_id);

            /* Finish the task. */
            R_TRY(m_rpc_client->End<rpc::ReceiveTask>(task_id, std::addressof(err), out_size));

            /* Check that our receive succeeded. */
            R_TRY(recv_result);
        } else {
            /* Finish the task. */
            R_TRY(m_rpc_client->End<rpc::ReceiveTask>(task_id, std::addressof(err), out_size));
        }

        /* Set output. */
        *out_err = err;
        return ResultSuccess();
    }

    Result HtcsService::SelectStart(u32 *out_task_id, os::NativeHandle *out_handle, Span<const int> read_handles, Span<const int> write_handles, Span<const int> exception_handles, s64 tv_sec, s64 tv_usec) {
        /* Begin the task. */
        u32 task_id;
        R_TRY(m_rpc_client->Begin<rpc::SelectTask>(std::addressof(task_id), read_handles, write_handles, exception_handles, tv_sec, tv_usec));

        /* Detach the task. */
        *out_task_id = task_id;
        *out_handle  = m_rpc_client->DetachReadableHandle(task_id);

        /* Check that the task isn't cancelled. */
        R_UNLESS(!m_rpc_client->IsCancelled<rpc::SelectTask>(task_id), htcs::ResultCancelled());

        return ResultSuccess();
    }

    Result HtcsService::SelectEnd(s32 *out_err, bool *out_empty, Span<int> read_handles, Span<int> write_handles, Span<int> exception_handles, u32 task_id) {
        /* Finish the task. */
        htcs::SocketError err;
        bool empty;
        R_TRY(m_rpc_client->End<rpc::SelectTask>(task_id, std::addressof(err), std::addressof(empty), read_handles, write_handles, exception_handles));

        /* Set output. */
        *out_err   = err;
        *out_empty = empty;
        return ResultSuccess();
    }

}
