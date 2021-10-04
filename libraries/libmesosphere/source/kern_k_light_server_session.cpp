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

    namespace {

        constexpr u64 InvalidThreadId = -1ull;

        class ThreadQueueImplForKLightServerSessionRequest final : public KThreadQueue {
            private:
                KThread::WaiterList *m_wait_list;
            public:
                constexpr ThreadQueueImplForKLightServerSessionRequest(KThread::WaiterList *wl) : KThreadQueue(), m_wait_list(wl) { /* ... */ }

                virtual void EndWait(KThread *waiting_thread, Result wait_result) override {
                    /* Remove the thread from our wait list. */
                    m_wait_list->erase(m_wait_list->iterator_to(*waiting_thread));

                    /* Invoke the base end wait handler. */
                    KThreadQueue::EndWait(waiting_thread, wait_result);
                }

                virtual void CancelWait(KThread *waiting_thread, Result wait_result, bool cancel_timer_task) override {
                    /* Remove the thread from our wait list. */
                    m_wait_list->erase(m_wait_list->iterator_to(*waiting_thread));

                    /* Invoke the base cancel wait handler. */
                    KThreadQueue::CancelWait(waiting_thread, wait_result, cancel_timer_task);
                }
        };

        class ThreadQueueImplForKLightServerSessionReceive final : public KThreadQueue {
            private:
                KThread **m_server_thread;
            public:
                constexpr ThreadQueueImplForKLightServerSessionReceive(KThread **st) : KThreadQueue(), m_server_thread(st) { /* ... */ }

                virtual void EndWait(KThread *waiting_thread, Result wait_result) override {
                    /* Clear the server thread. */
                    *m_server_thread = nullptr;

                    /* Set the waiting thread as not cancelable. */
                    waiting_thread->ClearCancellable();

                    /* Invoke the base end wait handler. */
                    KThreadQueue::EndWait(waiting_thread, wait_result);
                }

                virtual void CancelWait(KThread *waiting_thread, Result wait_result, bool cancel_timer_task) override {
                    /* Clear the server thread. */
                    *m_server_thread = nullptr;

                    /* Set the waiting thread as not cancelable. */
                    waiting_thread->ClearCancellable();

                    /* Invoke the base cancel wait handler. */
                    KThreadQueue::CancelWait(waiting_thread, wait_result, cancel_timer_task);
                }
        };

    }

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

        ThreadQueueImplForKLightServerSessionRequest wait_queue(std::addressof(m_request_list));

        /* Send the request. */
        {
            /* Lock the scheduler. */
            KScopedSchedulerLock sl;

            /* Check that the server isn't closed. */
            R_UNLESS(!m_parent->IsServerClosed(), svc::ResultSessionClosed());

            /* Check that the request thread isn't terminating. */
            R_UNLESS(!request_thread->IsTerminationRequested(), svc::ResultTerminationRequested());

            /* Add the request thread to our list. */
            m_request_list.push_back(*request_thread);

            /* Begin waiting on the request. */
            request_thread->BeginWait(std::addressof(wait_queue));

            /* If we have a server thread, end its wait. */
            if (m_server_thread != nullptr) {
                m_server_thread->EndWait(ResultSuccess());
            }
        }

        /* NOTE: Nintendo returns GetCurrentThread().GetWaitResult() here. */
        /* This is technically incorrect, although it doesn't cause problems in practice */
        /* because this is only ever called with request_thread = GetCurrentThreadPointer(). */
        return request_thread->GetWaitResult();
    }

    Result KLightServerSession::ReplyAndReceive(u32 *data) {
        MESOSPHERE_ASSERT_THIS();

        /* Set the server context. */
        GetCurrentThread().SetLightSessionData(data);

        /* Reply, if we need to. */
        if (data[0] & KLightSession::ReplyFlag) {
            KScopedSchedulerLock sl;

            /* Check that we're open. */
            R_UNLESS(!m_parent->IsClientClosed(), svc::ResultSessionClosed());
            R_UNLESS(!m_parent->IsServerClosed(), svc::ResultSessionClosed());

            /* Check that we have a request to reply to. */
            R_UNLESS(m_current_request != nullptr, svc::ResultInvalidState());

            /* Check that the server thread id is correct. */
            R_UNLESS(m_server_thread_id == GetCurrentThread().GetId(), svc::ResultInvalidState());

            /* If we can reply, do so. */
            if (!m_current_request->IsTerminationRequested()) {
                std::memcpy(m_current_request->GetLightSessionData(), GetCurrentThread().GetLightSessionData(), KLightSession::DataSize);
                m_current_request->EndWait(ResultSuccess());
            }

            /* Close our current request. */
            m_current_request->Close();

            /* Clear our current request. */
            m_current_request  = nullptr;
            m_server_thread_id = InvalidThreadId;
        }

        /* Close any pending objects before we wait. */
        GetCurrentThread().DestroyClosedObjects();

        /* Create the wait queue for our receive. */
        ThreadQueueImplForKLightServerSessionReceive wait_queue(std::addressof(m_server_thread));

        /* Receive. */
        while (true) {
            /* Try to receive a request. */
            {
                KScopedSchedulerLock sl;

                /* Check that we aren't already receiving. */
                R_UNLESS(m_server_thread == nullptr,            svc::ResultInvalidState());
                R_UNLESS(m_server_thread_id == InvalidThreadId, svc::ResultInvalidState());

                /* Check that we're open. */
                R_UNLESS(!m_parent->IsClientClosed(), svc::ResultSessionClosed());
                R_UNLESS(!m_parent->IsServerClosed(), svc::ResultSessionClosed());

                /* Check that we're not terminating. */
                R_UNLESS(!GetCurrentThread().IsTerminationRequested(), svc::ResultTerminationRequested());

                /* If we have a request available, use it. */
                if (auto head = m_request_list.begin(); head != m_request_list.end()) {
                    /* Set our current request. */
                    m_current_request = std::addressof(*head);
                    m_current_request->Open();

                    /* Set our server thread id. */
                    m_server_thread_id = GetCurrentThread().GetId();

                    /* Copy the client request data. */
                    std::memcpy(GetCurrentThread().GetLightSessionData(), m_current_request->GetLightSessionData(), KLightSession::DataSize);

                    /* We successfully received. */
                    return ResultSuccess();
                }

                /* We need to wait for a request to come in. */

                /* Check if we were cancelled. */
                if (GetCurrentThread().IsWaitCancelled()) {
                    GetCurrentThread().ClearWaitCancelled();
                    return svc::ResultCancelled();
                }

                /* Mark ourselves as cancellable. */
                GetCurrentThread().SetCancellable();

                /* Wait for a request to come in. */
                m_server_thread = GetCurrentThreadPointer();
                GetCurrentThread().BeginWait(std::addressof(wait_queue));
            }

            /* We waited to receive a request; if our wait failed, return the failing result. */
            R_TRY(GetCurrentThread().GetWaitResult());
        }
    }

    void KLightServerSession::CleanupRequests() {
        /* Cleanup all pending requests. */
        {
            KScopedSchedulerLock sl;

            /* Handle the current request. */
            if (m_current_request != nullptr) {
                /* Reply to the current request. */
                if (!m_current_request->IsTerminationRequested()) {
                    m_current_request->EndWait(svc::ResultSessionClosed());
                }

                /* Clear our current request. */
                m_current_request->Close();
                m_current_request  = nullptr;
                m_server_thread_id = InvalidThreadId;
            }

            /* Reply to all other requests. */
            for (auto &thread : m_request_list) {
                thread.EndWait(svc::ResultSessionClosed());
            }

            /* Wait up our server thread, if we have one. */
            if (m_server_thread != nullptr) {
                m_server_thread->EndWait(svc::ResultSessionClosed());
            }
        }
    }

}
