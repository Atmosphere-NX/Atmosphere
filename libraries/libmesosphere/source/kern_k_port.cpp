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

    void KPort::Initialize(s32 max_sessions, bool is_light, uintptr_t name) {
        /* Open a new reference count to the initialized port. */
        this->Open();

        /* Create and initialize our server/client pair. */
        KAutoObject::Create<KServerPort>(std::addressof(m_server));
        KAutoObject::Create<KClientPort>(std::addressof(m_client));
        m_server.Initialize(this);
        m_client.Initialize(this, max_sessions);

        /* Set our member variables. */
        m_is_light = is_light;
        m_name     = name;
        m_state    = State::Normal;
    }

    void KPort::OnClientClosed() {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock sl;

        if (m_state == State::Normal) {
            m_state = State::ClientClosed;
        }
    }

    void KPort::OnServerClosed() {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock sl;

        if (m_state == State::Normal) {
            m_state = State::ServerClosed;
        }
    }

    Result KPort::EnqueueSession(KServerSession *session) {
        KScopedSchedulerLock sl;

        R_UNLESS(m_state == State::Normal, svc::ResultPortClosed());

        m_server.EnqueueSession(session);
        R_SUCCEED();
    }

    Result KPort::EnqueueSession(KLightServerSession *session) {
        KScopedSchedulerLock sl;

        R_UNLESS(m_state == State::Normal, svc::ResultPortClosed());

        m_server.EnqueueSession(session);
        R_SUCCEED();
    }

}
