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
#include "htcs_rpc_tasks.hpp"

namespace ams::htcs::impl::rpc {

    Result CloseTask::SetArguments(s32 handle) {
        /* Set our arguments. */
        m_handle = handle;

        return ResultSuccess();
    }

    void CloseTask::Complete(htcs::SocketError err) {
        /* Set our results. */
        m_err  = err;

        /* Complete. */
        HtcsTask::Complete();
    }

    Result CloseTask::GetResult(htcs::SocketError *out_err) const {
        /* Sanity check our state. */
        AMS_ASSERT(this->GetTaskState() == htc::server::rpc::RpcTaskState::Completed);

        /* Set the output. */
        *out_err  = m_err;

        return ResultSuccess();
    }

    Result CloseTask::ProcessResponse(const char *data, size_t size) {
        /* Convert the input to a packet. */
        auto *packet = reinterpret_cast<const HtcsRpcPacket *>(data);

        /* Complete the task. */
        this->Complete(static_cast<htcs::SocketError>(packet->params[0]));

        return ResultSuccess();
    }

    Result CloseTask::CreateRequest(size_t *out, char *data, size_t size, u32 task_id) {
        /* Create the packet. */
        auto *packet = reinterpret_cast<HtcsRpcPacket *>(data);
        *packet = {
            .protocol  = HtcsProtocol,
            .version   = this->GetVersion(),
            .category  = HtcsPacketCategory::Request,
            .type      = HtcsPacketType::Close,
            .body_size = 0,
            .task_id   = task_id,
            .params    = {
                m_handle,
            },
        };

        /* Set the output size. */
        *out = sizeof(*packet);

        return ResultSuccess();
    }

}
