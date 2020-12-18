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

    void KLightServerSession::Destroy() {
        MESOSPHERE_ASSERT_THIS();

        this->CleanupRequests();

        m_parent->OnServerClosed();
    }

    void KLightServerSession::OnClientClosed() {
        MESOSPHERE_ASSERT_THIS();

        this->CleanupRequests();
    }

    Result KLightServerSession::OnRequest(KThread *request_thread) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Check that the server isn't closed. */
        R_UNLESS(!m_parent->IsServerClosed(), svc::ResultSessionClosed());

        /* Try to sleep the thread. */
        R_UNLESS(m_request_queue.SleepThread(request_thread), svc::ResultTerminationRequested());

        /* If we don't have a current request, wake up a server thread to handle it. */
        if (m_current_request == nullptr) {
            m_server_queue.WakeupFrontThread();
        }

        return ResultSuccess();
    }

    Result KLightServerSession::ReplyAndReceive(u32 *data) {
        MESOSPHERE_ASSERT_THIS();

        /* Set the server context. */
        KThread *server_thread = GetCurrentThreadPointer();
        server_thread->SetLightSessionData(data);

        /* Reply, if we need to. */
        KThread *cur_request = nullptr;
        if (data[0] & KLightSession::ReplyFlag) {
            KScopedSchedulerLock sl;

            /* Check that we're open. */
            R_UNLESS(!m_parent->IsClientClosed(), svc::ResultSessionClosed());
            R_UNLESS(!m_parent->IsServerClosed(), svc::ResultSessionClosed());

            /* Check that we have a request to reply to. */
            R_UNLESS(m_current_request != nullptr, svc::ResultInvalidState());

            /* Check that the server thread is correct. */
            R_UNLESS(m_server_thread == server_thread, svc::ResultInvalidState());

            /* If we can reply, do so. */
            if (!m_current_request->IsTerminationRequested()) {
                MESOSPHERE_ASSERT(m_current_request->GetState() == KThread::ThreadState_Waiting);
                MESOSPHERE_ASSERT(m_request_queue.begin() != m_request_queue.end() && m_current_request == std::addressof(*m_request_queue.begin()));
                std::memcpy(m_current_request->GetLightSessionData(), server_thread->GetLightSessionData(), KLightSession::DataSize);
                m_request_queue.WakeupThread(m_current_request);
            }

            /* Clear our current request. */
            cur_request = m_current_request;
            m_current_request = nullptr;
            m_server_thread   = nullptr;
        }

        /* Close the current request, if we had one. */
        if (cur_request != nullptr) {
            cur_request->Close();
        }

        /* Receive. */
        bool set_cancellable = false;
        while (true) {
            KScopedSchedulerLock sl;

            /* Check that we aren't already receiving. */
            R_UNLESS(m_server_queue.IsEmpty(),   svc::ResultInvalidState());
            R_UNLESS(m_server_thread == nullptr, svc::ResultInvalidState());

            /* If we cancelled in a previous loop, clear cancel state. */
            if (set_cancellable) {
                server_thread->ClearCancellable();
                set_cancellable = false;
            }

            /* Check that we're open. */
            R_UNLESS(!m_parent->IsClientClosed(), svc::ResultSessionClosed());
            R_UNLESS(!m_parent->IsServerClosed(), svc::ResultSessionClosed());

            /* If we have a request available, use it. */
            if (m_current_request == nullptr && !m_request_queue.IsEmpty()) {
                m_current_request = std::addressof(*m_request_queue.begin());
                m_current_request->Open();
                m_server_thread   = server_thread;
                break;
            } else {
                /* Otherwise, wait for a request to come in. */
                R_UNLESS(m_server_queue.SleepThread(server_thread), svc::ResultTerminationRequested());

                /* Check if we were cancelled. */
                if (server_thread->IsWaitCancelled()) {
                    m_server_queue.WakeupThread(server_thread);
                    server_thread->ClearWaitCancelled();
                    return svc::ResultCancelled();
                }

                /* Otherwise, mark as cancellable. */
                server_thread->SetCancellable();
                set_cancellable = true;
            }
        }

        /* Copy the client data. */
        std::memcpy(server_thread->GetLightSessionData(), m_current_request->GetLightSessionData(), KLightSession::DataSize);
        return ResultSuccess();
    }

    void KLightServerSession::CleanupRequests() {
        /* Cleanup all pending requests. */
        KThread *cur_request = nullptr;
        {
            KScopedSchedulerLock sl;

            /* Handle the current request. */
            if (m_current_request != nullptr) {
                /* Reply to the current request. */
                if (!m_current_request->IsTerminationRequested()) {
                    MESOSPHERE_ASSERT(m_current_request->GetState() == KThread::ThreadState_Waiting);
                    MESOSPHERE_ASSERT(m_request_queue.begin() != m_request_queue.end() && m_current_request == std::addressof(*m_request_queue.begin()));
                    m_request_queue.WakeupThread(m_current_request);
                    m_current_request->SetSyncedObject(nullptr, svc::ResultSessionClosed());
                }

                /* Clear our current request. */
                cur_request = m_current_request;
                m_current_request = nullptr;
                m_server_thread   = nullptr;
            }

            /* Reply to all other requests. */
            while (!m_request_queue.IsEmpty()) {
                KThread *client_thread = m_request_queue.WakeupFrontThread();
                client_thread->SetSyncedObject(nullptr, svc::ResultSessionClosed());
            }

            /* Wake up all server threads. */
            while (!m_server_queue.IsEmpty()) {
                m_server_queue.WakeupFrontThread();
            }
        }

        /* Close the current request, if we had one. */
        if (cur_request != nullptr) {
            cur_request->Close();
        }
    }

}
