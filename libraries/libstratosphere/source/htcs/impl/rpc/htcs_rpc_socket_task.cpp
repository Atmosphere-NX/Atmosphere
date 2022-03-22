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

    namespace {

        constinit s16 g_protocol_version = HtcsMaxVersion;

    }

    HtcsTask::HtcsTask(HtcsTaskType type) : m_task_type(type), m_version(g_protocol_version) {
        /* ... */
    }

    Result SocketTask::SetArguments() {
        return ResultSuccess();
    }

    void SocketTask::Complete(htcs::SocketError err, s32 desc) {
        /* Set our results. */
        m_err  = err;
        m_desc = desc;

        /* Complete. */
        HtcsTask::Complete();
    }

    Result SocketTask::GetResult(htcs::SocketError *out_err, s32 *out_desc) const {
        /* Sanity check our state. */
        AMS_ASSERT(this->GetTaskState() == htc::server::rpc::RpcTaskState::Completed);

        /* Set the output. */
        *out_err  = m_err;
        *out_desc = m_desc;

        return ResultSuccess();
    }

    Result SocketTask::ProcessResponse(const char *data, size_t size) {
        AMS_UNUSED(size);

        /* Convert the input to a packet. */
        auto *packet = reinterpret_cast<const HtcsRpcPacket *>(data);

        /* Update the global protocol version. */
        g_protocol_version = std::min(g_protocol_version, packet->version);

        /* Complete the task. */
        this->Complete(static_cast<htcs::SocketError>(packet->params[0]), packet->params[1]);

        return ResultSuccess();
    }

    Result SocketTask::CreateRequest(size_t *out, char *data, size_t size, u32 task_id) {
        AMS_UNUSED(size);

        /* Create the packet. */
        auto *packet = reinterpret_cast<HtcsRpcPacket *>(data);
        *packet = {
            .protocol  = HtcsProtocol,
            .version   = 3,
            .category  = HtcsPacketCategory::Request,
            .type      = HtcsPacketType::Socket,
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
