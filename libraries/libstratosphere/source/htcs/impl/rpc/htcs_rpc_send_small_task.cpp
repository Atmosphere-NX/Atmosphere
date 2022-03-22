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

    void SendSmallTask::SetBuffer(const void *buffer, s64 buffer_size) {
        /* Sanity check the buffer size. */
        AMS_ASSERT(0 <= buffer_size && buffer_size <= static_cast<s64>(sizeof(m_buffer)));

        /* Set our buffer. */
        if (buffer_size > 0) {
            std::memcpy(m_buffer, buffer, buffer_size);
        }
        m_buffer_size = buffer_size;
    }

    void SendSmallTask::NotifyDataChannelReady() {
        /* Notify. */
        this->Notify();

        /* Signal our ready event. */
        m_ready_event.Signal();
    }

    void SendSmallTask::WaitNotification() {
        /* Wait on our ready event. */
        m_ready_event.Wait();
    }

    Result SendSmallTask::SetArguments(s32 handle, s64 size, htcs::MessageFlag flags) {
        /* Check that we're valid. */
        R_UNLESS(this->IsValid(), htcs::ResultInvalidTask());

        /* Set our arguments. */
        m_handle = handle;
        m_size   = size;
        m_flags  = flags;

        return ResultSuccess();
    }

    void SendSmallTask::Complete(htcs::SocketError err, s64 size) {
        /* Set our results. */
        m_err         = err;
        m_result_size = size;

        /* Signal our ready event. */
        m_ready_event.Signal();

        /* Complete. */
        HtcsSignalingTask::Complete();
    }

    Result SendSmallTask::GetResult(htcs::SocketError *out_err, s64 *out_size) const {
        /* Sanity check our state. */
        AMS_ASSERT(this->GetTaskState() == htc::server::rpc::RpcTaskState::Completed);

        /* Set the output. */
        *out_err  = m_err;
        *out_size = m_result_size;

        return ResultSuccess();
    }

    void SendSmallTask::Cancel(htc::server::rpc::RpcTaskCancelReason reason) {
        /* Cancel the task. */
        HtcsSignalingTask::Cancel(reason);

        /* Signal our ready event. */
        m_ready_event.Signal();
    }

    Result SendSmallTask::ProcessResponse(const char *data, size_t size) {
        AMS_UNUSED(size);

        /* Convert the input to a packet. */
        auto *packet = reinterpret_cast<const HtcsRpcPacket *>(data);

        /* Complete the task. */
        this->Complete(static_cast<htcs::SocketError>(packet->params[0]), this->GetSize());

        return ResultSuccess();
    }

    Result SendSmallTask::CreateRequest(size_t *out, char *data, size_t size, u32 task_id) {
        AMS_UNUSED(size);

        /* Sanity check our size. */
        AMS_ASSERT(sizeof(HtcsRpcPacket) + this->GetBufferSize() <= size);

        /* Create the packet. */
        auto *packet = reinterpret_cast<HtcsRpcPacket *>(data);
        *packet = {
            .protocol  = HtcsProtocol,
            .version   = this->GetVersion(),
            .category  = HtcsPacketCategory::Request,
            .type      = HtcsPacketType::Send,
            .body_size = this->GetSize(),
            .task_id   = task_id,
            .params    = {
                m_handle,
                m_size,
                static_cast<s64>(m_flags),
            },
        };

        /* Set the body. */
        if (this->GetSize() > 0) {
            std::memcpy(packet->data, this->GetBuffer(), this->GetSize());
        }

        /* Set the output size. */
        *out = sizeof(*packet) + this->GetSize();

        return ResultSuccess();
    }

    bool SendSmallTask::IsSendBufferRequired() {
        return this->GetSize() > 0;
    }

}
