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

    Result AcceptTask::SetArguments(s32 server_handle) {
        /* Check that we're valid. */
        R_UNLESS(this->IsValid(), htcs::ResultInvalidTask());

        /* Set our arguments. */
        m_server_handle = server_handle;

        return ResultSuccess();
    }

    void AcceptTask::Complete(htcs::SocketError err, s32 desc) {
        /* Set our results. */
        m_err  = err;
        m_desc = desc;

        /* Complete. */
        HtcsSignalingTask::Complete();
    }

    Result AcceptTask::GetResult(htcs::SocketError *out_err, s32 *out_desc, s32 server_handle) const {
        /* Check the server handle. */
        R_UNLESS(m_server_handle == server_handle, htcs::ResultInvalidServerHandle());

        /* Sanity check our state. */
        AMS_ASSERT(this->GetTaskState() == htc::server::rpc::RpcTaskState::Completed);

        /* Set the output. */
        *out_err  = m_err;
        *out_desc = m_desc;

        return ResultSuccess();
    }

    Result AcceptTask::ProcessResponse(const char *data, size_t size) {
        AMS_UNUSED(size);

        /* Convert the input to a packet. */
        auto *packet = reinterpret_cast<const HtcsRpcPacket *>(data);

        /* Complete the task. */
        this->Complete(static_cast<htcs::SocketError>(packet->params[0]), packet->params[1]);

        return ResultSuccess();
    }

    Result AcceptTask::CreateRequest(size_t *out, char *data, size_t size, u32 task_id) {
        AMS_UNUSED(size);

        /* Create the packet. */
        auto *packet = reinterpret_cast<HtcsRpcPacket *>(data);
        *packet = {
            .protocol  = HtcsProtocol,
            .version   = this->GetVersion(),
            .category  = HtcsPacketCategory::Request,
            .type      = HtcsPacketType::Accept,
            .body_size = 0,
            .task_id   = task_id,
            .params    = {
                m_server_handle,
            },
        };

        /* Set the output size. */
        *out = sizeof(*packet);

        return ResultSuccess();
    }

}
