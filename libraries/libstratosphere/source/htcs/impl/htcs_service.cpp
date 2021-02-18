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
        *out_err  = err;
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
        *out_err  = err;
        return ResultSuccess();
    }

    Result HtcsService::Listen(s32 *out_err, s32 desc, s32 backlog_count);
    Result HtcsService::Receive(s32 *out_err, s64 *out_size, char *buffer, size_t size, s32 desc, s32 flags);
    Result HtcsService::Send(s32 *out_err, s64 *out_size, const char *buffer, size_t size, s32 desc, s32 flags);
    Result HtcsService::Shutdown(s32 *out_err, s32 desc, s32 how);
    Result HtcsService::Fcntl(s32 *out_err, s32 *out_res, s32 desc, s32 command, s32 value);

    Result HtcsService::AcceptStart(u32 *out_task_id, Handle *out_handle, s32 desc);
    Result HtcsService::AcceptResults(s32 *out_err, s32 *out_desc, SockAddrHtcs *out_address, u32 task_id, s32 desc);

    Result HtcsService::ReceiveSmallStart(u32 *out_task_id, Handle *out_handle, s64 size, s32 desc, s32 flags);
    Result HtcsService::ReceiveSmallResults(s32 *out_err, s64 *out_size, char *buffer, s64 buffer_size, u32 task_id, s32 desc);

    Result HtcsService::SendSmallStart(u32 *out_task_id, Handle *out_handle, s32 desc, s64 size, s32 flags);
    Result HtcsService::SendSmallContinue(s64 *out_size, const char *buffer, s64 buffer_size, u32 task_id, s32 desc);
    Result HtcsService::SendSmallResults(s32 *out_err, s64 *out_size, u32 task_id, s32 desc);

    Result HtcsService::SendStart(u32 *out_task_id, Handle *out_handle, s32 desc, s64 size, s32 flags);
    Result HtcsService::SendContinue(s64 *out_size, const char *buffer, s64 buffer_size, u32 task_id, s32 desc);
    Result HtcsService::SendResults(s32 *out_err, s64 *out_size, u32 task_id, s32 desc);

    Result HtcsService::ReceiveStart(u32 *out_task_id, Handle *out_handle, s64 size, s32 desc, s32 flags);
    Result HtcsService::ReceiveResults(s32 *out_err, s64 *out_size, char *buffer, s64 buffer_size, u32 task_id, s32 desc);

    Result HtcsService::SelectStart(u32 *out_task_id, Handle *out_handle, Span<const int> read_handles, Span<const int> write_handles, Span<const int> exception_handles, s64 tv_sec, s64 tv_usec);
    Result HtcsService::SelectEnd(s32 *out_err, s32 *out_res, Span<int> read_handles, Span<int> write_handles, Span<int> exception_handles, u32 task_id);

}
