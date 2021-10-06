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
#include "htcs_rpc_tasks.hpp"

namespace ams::htcs::impl::rpc {

    void SendTask::SetBuffer(const void *buffer, s64 buffer_size) {
        /* Set our buffer. */
        m_buffer      = buffer;
        m_buffer_size = buffer_size;
    }

    void SendTask::NotifyDataChannelReady() {
        /* Notify. */
        this->Notify();

        /* Signal our ready event. */
        m_ready_event.Signal();
    }

    void SendTask::WaitNotification() {
        /* Wait on our ready event. */
        m_ready_event.Wait();
    }

    Result SendTask::SetArguments(s32 handle, s64 size, htcs::MessageFlag flags) {
        /* Check that we're valid. */
        R_UNLESS(this->IsValid(), htcs::ResultInvalidTask());

        /* Set our arguments. */
        m_handle = handle;
        m_size   = size;
        m_flags  = flags;

        return ResultSuccess();
    }

    void SendTask::Complete(htcs::SocketError err, s64 size) {
        /* Set our results. */
        m_err         = err;
        m_result_size = size;

        /* Signal our ready event. */
        m_ready_event.Signal();

        /* Complete. */
        HtcsSignalingTask::Complete();
    }

    Result SendTask::GetResult(htcs::SocketError *out_err, s64 *out_size) const {
        /* Sanity check our state. */
        AMS_ASSERT(this->GetTaskState() == htc::server::rpc::RpcTaskState::Completed);

        /* Set the output. */
        *out_err  = m_err;
        *out_size = m_result_size;

        return ResultSuccess();
    }

    void SendTask::Cancel(htc::server::rpc::RpcTaskCancelReason reason) {
        /* Cancel the task. */
        HtcsSignalingTask::Cancel(reason);

        /* Signal our ready event. */
        m_ready_event.Signal();
    }

    Result SendTask::ProcessResponse(const char *data, size_t size) {
        AMS_UNUSED(size);

        /* Convert the input to a packet. */
        auto *packet = reinterpret_cast<const HtcsRpcPacket *>(data);

        /* Complete the task. */
        this->Complete(static_cast<htcs::SocketError>(packet->params[0]), packet->params[1]);

        return ResultSuccess();
    }

    Result SendTask::CreateRequest(size_t *out, char *data, size_t size, u32 task_id) {
        AMS_UNUSED(size);

        /* Create the packet. */
        auto *packet = reinterpret_cast<HtcsRpcPacket *>(data);
        *packet = {
            .protocol  = HtcsProtocol,
            .version   = this->GetVersion(),
            .category  = HtcsPacketCategory::Request,
            .type      = HtcsPacketType::SendLarge,
            .body_size = 0,
            .task_id   = task_id,
            .params    = {
                m_handle,
                m_size,
                static_cast<s64>(m_flags),
                GetSendDataChannelId(task_id),
            },
        };

        /* Set the output size. */
        *out = sizeof(*packet);

        return ResultSuccess();
    }

    Result SendTask::ProcessNotification(const char *data, size_t size) {
        AMS_UNUSED(data, size);

        this->NotifyDataChannelReady();
        return ResultSuccess();
    }

    Result SendTask::CreateNotification(size_t *out, char *data, size_t size, u32 task_id) {
        AMS_UNUSED(size);

        /* Create the packet. */
        auto *packet = reinterpret_cast<HtcsRpcPacket *>(data);
        *packet = {
            .protocol  = HtcsProtocol,
            .version   = this->GetVersion(),
            .category  = HtcsPacketCategory::Notification,
            .type      = HtcsPacketType::SendLarge,
            .body_size = 0,
            .task_id   = task_id,
            .params    = {
                /* ... */
            },
        };

        /* Set the output size. */
        *out = sizeof(*packet);

        return ResultSuccess();
    }

}
