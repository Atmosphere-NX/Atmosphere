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
#include <mesosphere.hpp>

namespace ams::kern {

    void KLightClientSession::Destroy() {
        MESOSPHERE_ASSERT_THIS();

        m_parent->OnClientClosed();
    }

    void KLightClientSession::OnServerClosed() {
        MESOSPHERE_ASSERT_THIS();
    }

    Result KLightClientSession::SendSyncRequest(u32 *data) {
        MESOSPHERE_ASSERT_THIS();

        /* Get the request thread. */
        KThread *cur_thread = GetCurrentThreadPointer();

        /* Set the light data. */
        cur_thread->SetLightSessionData(data);

        /* Send the request. */
        {
            KScopedSchedulerLock sl;

            cur_thread->SetSyncedObject(nullptr, ResultSuccess());

            R_TRY(m_parent->OnRequest(cur_thread));
        }

        /* Get the result. */
        KSynchronizationObject *dummy;
        return cur_thread->GetWaitResult(std::addressof(dummy));
    }

}
