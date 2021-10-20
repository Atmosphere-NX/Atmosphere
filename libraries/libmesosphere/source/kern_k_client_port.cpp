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

    void KClientPort::Initialize(KPort *parent, s32 max_sessions) {
        /* Set member variables. */
        m_num_sessions  = 0;
        m_peak_sessions = 0;
        m_parent        = parent;
        m_max_sessions  = max_sessions;
    }

    void KClientPort::OnSessionFinalized() {
        KScopedSchedulerLock sl;

        if (const auto prev = m_num_sessions--; prev == m_max_sessions) {
            this->NotifyAvailable();
        }
    }

    void KClientPort::OnServerClosed() {
        MESOSPHERE_ASSERT_THIS();
    }

    bool KClientPort::IsLight() const {
        return this->GetParent()->IsLight();
    }

    bool KClientPort::IsServerClosed() const {
        return this->GetParent()->IsServerClosed();
    }

    void KClientPort::Destroy() {
        /* Note with our parent that we're closed. */
        m_parent->OnClientClosed();

        /* Close our reference to our parent. */
        m_parent->Close();
    }

    bool KClientPort::IsSignaled() const {
        MESOSPHERE_ASSERT_THIS();
        return m_num_sessions.Load() < m_max_sessions;
    }

    Result KClientPort::CreateSession(KClientSession **out) {
        MESOSPHERE_ASSERT_THIS();

        /* Declare the session we're going to allocate. */
        KSession *session;

        /* Reserve a new session from the resource limit. */
        KScopedResourceReservation session_reservation(GetCurrentProcessPointer(), ams::svc::LimitableResource_SessionCountMax);
        if (session_reservation.Succeeded()) {
            /* Allocate a session normally. */
            session = KSession::Create();
        } else {
            /* We couldn't reserve a session. Check that we support dynamically expanding the resource limit. */
            R_UNLESS(GetCurrentProcess().GetResourceLimit() == std::addressof(Kernel::GetSystemResourceLimit()), svc::ResultLimitReached());
            R_UNLESS(KTargetSystem::IsDynamicResourceLimitsEnabled(),                                            svc::ResultLimitReached());

            /* Try to allocate a session from unused slab memory. */
            session = KSession::CreateFromUnusedSlabMemory();
            R_UNLESS(session != nullptr, svc::ResultLimitReached());

            /* Ensure that if we fail to allocate our session requests, we close the session we created. */
            auto session_guard = SCOPE_GUARD { session->Close(); };
            {
                /* We want to add two KSessionRequests to the heap, to prevent request exhaustion. */
                for (size_t i = 0; i < 2; ++i) {
                    KSessionRequest *request = KSessionRequest::CreateFromUnusedSlabMemory();
                    R_UNLESS(request != nullptr, svc::ResultLimitReached());

                    request->Close();
                }
            }
            session_guard.Cancel();

            /* We successfully allocated a session, so add the object we allocated to the resource limit. */
            Kernel::GetSystemResourceLimit().Add(ams::svc::LimitableResource_SessionCountMax, 1);
        }

        /* Check that we successfully created a session. */
        R_UNLESS(session != nullptr, svc::ResultOutOfResource());

        /* Update the session counts. */
        auto count_guard = SCOPE_GUARD { session->Close(); };
        {
            /* Atomically increment the number of sessions. */
            s32 new_sessions;
            {
                const auto max = m_max_sessions;
                auto cur_sessions = m_num_sessions.Load();
                do {
                    R_UNLESS(cur_sessions < max, svc::ResultOutOfSessions());
                    new_sessions = cur_sessions + 1;
                } while (!m_num_sessions.CompareExchangeWeak<std::memory_order_relaxed>(cur_sessions, new_sessions));

            }

            /* Atomically update the peak session tracking. */
            {
                auto peak = m_peak_sessions.Load();
                do {
                    if (peak >= new_sessions) {
                        break;
                    }
                } while (!m_peak_sessions.CompareExchangeWeak<std::memory_order_relaxed>(peak, new_sessions));
            }
        }
        count_guard.Cancel();

        /* Initialize the session. */
        session->Initialize(this, m_parent->GetName());

        /* Commit the session reservation. */
        session_reservation.Commit();

        /* Register the session. */
        KSession::Register(session);
        auto session_guard = SCOPE_GUARD {
            session->GetClientSession().Close();
            session->GetServerSession().Close();
        };

        /* Enqueue the session with our parent. */
        R_TRY(m_parent->EnqueueSession(std::addressof(session->GetServerSession())));

        /* We succeeded, so set the output. */
        session_guard.Cancel();
        *out = std::addressof(session->GetClientSession());
        return ResultSuccess();
    }

    Result KClientPort::CreateLightSession(KLightClientSession **out) {
        MESOSPHERE_ASSERT_THIS();

        /* Declare the session we're going to allocate. */
        KLightSession *session;

        /* Reserve a new session from the resource limit. */
        KScopedResourceReservation session_reservation(GetCurrentProcessPointer(), ams::svc::LimitableResource_SessionCountMax);
        if (session_reservation.Succeeded()) {
            /* Allocate a session normally. */
            session = KLightSession::Create();
        } else {
            /* We couldn't reserve a session. Check that we support dynamically expanding the resource limit. */
            R_UNLESS(GetCurrentProcess().GetResourceLimit() == std::addressof(Kernel::GetSystemResourceLimit()), svc::ResultLimitReached());
            R_UNLESS(KTargetSystem::IsDynamicResourceLimitsEnabled(),                                            svc::ResultLimitReached());

            /* Try to allocate a session from unused slab memory. */
            session = KLightSession::CreateFromUnusedSlabMemory();
            R_UNLESS(session != nullptr, svc::ResultLimitReached());

            /* We successfully allocated a session, so add the object we allocated to the resource limit. */
            Kernel::GetSystemResourceLimit().Add(ams::svc::LimitableResource_SessionCountMax, 1);
        }

        /* Check that we successfully created a session. */
        R_UNLESS(session != nullptr, svc::ResultOutOfResource());

        /* Update the session counts. */
        auto count_guard = SCOPE_GUARD { session->Close(); };
        {
            /* Atomically increment the number of sessions. */
            s32 new_sessions;
            {
                const auto max = m_max_sessions;
                auto cur_sessions = m_num_sessions.Load();
                do {
                    R_UNLESS(cur_sessions < max, svc::ResultOutOfSessions());
                    new_sessions = cur_sessions + 1;
                } while (!m_num_sessions.CompareExchangeWeak<std::memory_order_relaxed>(cur_sessions, new_sessions));

            }

            /* Atomically update the peak session tracking. */
            {
                auto peak = m_peak_sessions.Load();
                do {
                    if (peak >= new_sessions) {
                        break;
                    }
                } while (!m_peak_sessions.CompareExchangeWeak<std::memory_order_relaxed>(peak, new_sessions));
            }
        }
        count_guard.Cancel();

        /* Initialize the session. */
        session->Initialize(this, m_parent->GetName());

        /* Commit the session reservation. */
        session_reservation.Commit();

        /* Register the session. */
        KLightSession::Register(session);
        auto session_guard = SCOPE_GUARD {
            session->GetClientSession().Close();
            session->GetServerSession().Close();
        };

        /* Enqueue the session with our parent. */
        R_TRY(m_parent->EnqueueSession(std::addressof(session->GetServerSession())));

        /* We succeeded, so set the output. */
        session_guard.Cancel();
        *out = std::addressof(session->GetClientSession());
        return ResultSuccess();
    }

}
