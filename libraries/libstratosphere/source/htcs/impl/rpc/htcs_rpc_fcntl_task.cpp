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

    Result FcntlTask::SetArguments(s32 handle, s32 command, s32 value) {
        /* Set our arguments. */
        m_handle  = handle;
        m_command = command;
        m_value   = value;

        return ResultSuccess();
    }

    void FcntlTask::Complete(htcs::SocketError err, s32 res) {
        /* Set our results. */
        m_err = err;
        m_res = res;

        /* Complete. */
        HtcsTask::Complete();
    }

    Result FcntlTask::GetResult(htcs::SocketError *out_err, s32 *out_res) const {
        /* Sanity check our state. */
        AMS_ASSERT(this->GetTaskState() == htc::server::rpc::RpcTaskState::Completed);

        /* Set the output. */
        *out_err = m_err;
        *out_res = m_res;

        return ResultSuccess();
    }

    Result FcntlTask::ProcessResponse(const char *data, size_t size) {
        AMS_UNUSED(size);

        /* Convert the input to a packet. */
        auto *packet = reinterpret_cast<const HtcsRpcPacket *>(data);

        /* Complete the task. */
        this->Complete(static_cast<htcs::SocketError>(packet->params[0]), packet->params[1]);

        return ResultSuccess();
    }

    Result FcntlTask::CreateRequest(size_t *out, char *data, size_t size, u32 task_id) {
        AMS_UNUSED(size);

        /* Create the packet. */
        auto *packet = reinterpret_cast<HtcsRpcPacket *>(data);
        *packet = {
            .protocol  = HtcsProtocol,
            .version   = this->GetVersion(),
            .category  = HtcsPacketCategory::Request,
            .type      = HtcsPacketType::Fcntl,
            .body_size = 0,
            .task_id   = task_id,
            .params    = {
                m_handle,
                m_command,
                m_value,
            },
        };

        /* Set the output size. */
        *out = sizeof(*packet);

        return ResultSuccess();
    }

}
