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

    void KSession::Initialize(KClientPort *client_port, uintptr_t name) {
        MESOSPHERE_ASSERT_THIS();

        /* Increment reference count. */
        /* Because reference count is one on creation, this will result */
        /* in a reference count of two. Thus, when both server and client are closed */
        /* this object will be destroyed. */
        this->Open();

        /* Create our sub sessions. */
        KAutoObject::Create(std::addressof(m_server));
        KAutoObject::Create(std::addressof(m_client));

        /* Initialize our sub sessions. */
        m_server.Initialize(this);
        m_client.Initialize(this);

        /* Set state and name. */
        m_state = State::Normal;
        m_name  = name;

        /* Set our owner process. */
        m_process = GetCurrentProcessPointer();
        m_process->Open();

        /* Set our port. */
        m_port = client_port;
        if (m_port != nullptr) {
            m_port->Open();
        }

        /* Mark initialized. */
        m_initialized = true;
    }

    void KSession::Finalize() {
        if (m_port != nullptr) {
            m_port->OnSessionFinalized();
            m_port->Close();
        }
    }

    void KSession::OnServerClosed() {
        MESOSPHERE_ASSERT_THIS();

        if (m_state == State::Normal) {
            m_state = State::ServerClosed;
            m_client.OnServerClosed();
        }
    }

    void KSession::OnClientClosed() {
        MESOSPHERE_ASSERT_THIS();

        if (m_state == State::Normal) {
            m_state = State::ClientClosed;
            m_server.OnClientClosed();
        }
    }

    void KSession::PostDestroy(uintptr_t arg) {
        /* Release the session count resource the owner process holds. */
        KProcess *owner = reinterpret_cast<KProcess *>(arg);
        owner->ReleaseResource(ams::svc::LimitableResource_SessionCountMax, 1);
        owner->Close();
    }

}
