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

}
