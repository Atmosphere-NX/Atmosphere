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
#include <mesosphere.hpp>

namespace ams::kern {

    void KClientSession::Destroy() {
        MESOSPHERE_ASSERT_THIS();

        m_parent->OnClientClosed();
        m_parent->Close();
    }

    void KClientSession::OnServerClosed() {
        MESOSPHERE_ASSERT_THIS();
    }

    Result KClientSession::SendSyncRequest(uintptr_t address, size_t size) {
        MESOSPHERE_ASSERT_THIS();

        /* Create a session request. */
        KSessionRequest *request = KSessionRequest::Create();
        R_UNLESS(request != nullptr, svc::ResultOutOfResource());
        ON_SCOPE_EXIT { request->Close(); };

        /* Initialize the request. */
        request->Initialize(nullptr, address, size);

        /* Send the request. */
        R_RETURN(m_parent->OnRequest(request));
    }

    Result KClientSession::SendAsyncRequest(KEvent *event, uintptr_t address, size_t size) {
        MESOSPHERE_ASSERT_THIS();

        /* Create a session request. */
        KSessionRequest *request = KSessionRequest::Create();
        R_UNLESS(request != nullptr, svc::ResultOutOfResource());
        ON_SCOPE_EXIT { request->Close(); };

        /* Initialize the request. */
        request->Initialize(event, address, size);

        /* Send the request. */
        R_RETURN(m_parent->OnRequest(request));
    }

}
