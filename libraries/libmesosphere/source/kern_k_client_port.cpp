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

    void KClientPort::Initialize(KPort *parent, s32 max_sessions) {
        /* Set member variables. */
        this->num_sessions  = 0;
        this->peak_sessions = 0;
        this->parent        = parent;
        this->max_sessions  = max_sessions;
    }

    void KClientPort::OnSessionFinalized() {
        KScopedSchedulerLock sl;

        const auto prev = this->num_sessions--;
        if (prev == this->max_sessions) {
            this->NotifyAvailable();
        }
    }

    void KClientPort::OnServerClosed() {
        MESOSPHERE_ASSERT_THIS();
    }

    bool KClientPort::IsLight() const {
        return this->GetParent()->IsLight();
    }

    void KClientPort::Destroy() {
        /* Note with our parent that we're closed. */
        this->parent->OnClientClosed();

        /* Close our reference to our parent. */
        this->parent->Close();
    }

    bool KClientPort::IsSignaled() const {
        MESOSPHERE_ASSERT_THIS();
        return this->num_sessions < this->max_sessions;
    }

    Result KClientPort::CreateSession(KClientSession **out) {
        MESOSPHERE_ASSERT_THIS();

        /* Reserve a new session from the resource limit. */
        KScopedResourceReservation session_reservation(GetCurrentProcessPointer(), ams::svc::LimitableResource_SessionCountMax);
        R_UNLESS(session_reservation.Succeeded(), svc::ResultLimitReached());

        /* Update the session counts. */
        {
            /* Atomically increment the number of sessions. */
            s32 new_sessions;
            {
                const auto max = this->max_sessions;
                auto cur_sessions = this->num_sessions.load(std::memory_order_acquire);
                do {
                    R_UNLESS(cur_sessions < max, svc::ResultOutOfSessions());
                    new_sessions = cur_sessions + 1;
                } while (!this->num_sessions.compare_exchange_weak(cur_sessions, new_sessions, std::memory_order_relaxed));

            }

            /* Atomically update the peak session tracking. */
            {
                auto peak = this->peak_sessions.load(std::memory_order_acquire);
                do {
                    if (peak >= new_sessions) {
                        break;
                    }
                } while (!this->peak_sessions.compare_exchange_weak(peak, new_sessions, std::memory_order_relaxed));
            }
        }

        /* Create a new session. */
        KSession *session = KSession::Create();
        if (session == nullptr) {
            /* Decrement the session count. */
            const auto prev = this->num_sessions--;
            if (prev == this->max_sessions) {
                this->NotifyAvailable();
            }

            return svc::ResultOutOfResource();
        }

        /* Initialize the session. */
        session->Initialize(this, this->parent->GetName());

        /* Commit the session reservation. */
        session_reservation.Commit();

        /* Register the session. */
        KSession::Register(session);
        auto session_guard = SCOPE_GUARD {
            session->GetClientSession().Close();
            session->GetServerSession().Close();
        };

        /* Enqueue the session with our parent. */
        R_TRY(this->parent->EnqueueSession(std::addressof(session->GetServerSession())));

        /* We succeeded, so set the output. */
        session_guard.Cancel();
        *out = std::addressof(session->GetClientSession());
        return ResultSuccess();
    }

    Result KClientPort::CreateLightSession(KLightClientSession **out) {
        MESOSPHERE_ASSERT_THIS();

        /* Reserve a new session from the resource limit. */
        KScopedResourceReservation session_reservation(GetCurrentProcessPointer(), ams::svc::LimitableResource_SessionCountMax);
        R_UNLESS(session_reservation.Succeeded(), svc::ResultLimitReached());

        /* Update the session counts. */
        {
            /* Atomically increment the number of sessions. */
            s32 new_sessions;
            {
                const auto max = this->max_sessions;
                auto cur_sessions = this->num_sessions.load(std::memory_order_acquire);
                do {
                    R_UNLESS(cur_sessions < max, svc::ResultOutOfSessions());
                    new_sessions = cur_sessions + 1;
                } while (!this->num_sessions.compare_exchange_weak(cur_sessions, new_sessions, std::memory_order_relaxed));

            }

            /* Atomically update the peak session tracking. */
            {
                auto peak = this->peak_sessions.load(std::memory_order_acquire);
                do {
                    if (peak >= new_sessions) {
                        break;
                    }
                } while (!this->peak_sessions.compare_exchange_weak(peak, new_sessions, std::memory_order_relaxed));
            }
        }

        /* Create a new session. */
        KLightSession *session = KLightSession::Create();
        if (session == nullptr) {
            /* Decrement the session count. */
            const auto prev = this->num_sessions--;
            if (prev == this->max_sessions) {
                this->NotifyAvailable();
            }

            return svc::ResultOutOfResource();
        }

        /* Initialize the session. */
        session->Initialize(this, this->parent->GetName());

        /* Commit the session reservation. */
        session_reservation.Commit();

        /* Register the session. */
        KLightSession::Register(session);
        auto session_guard = SCOPE_GUARD {
            session->GetClientSession().Close();
            session->GetServerSession().Close();
        };

        /* Enqueue the session with our parent. */
        R_TRY(this->parent->EnqueueSession(std::addressof(session->GetServerSession())));

        /* We succeeded, so set the output. */
        session_guard.Cancel();
        *out = std::addressof(session->GetClientSession());
        return ResultSuccess();
    }

}
