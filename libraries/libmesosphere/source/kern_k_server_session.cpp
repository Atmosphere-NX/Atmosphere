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

    namespace {

        ALWAYS_INLINE void ReplyAsyncError(KProcess *to_process, uintptr_t to_msg_buf, size_t to_msg_buf_size, Result result) {
            /* Convert the buffer to a physical address. */
            KPhysicalAddress phys_addr;
            to_process->GetPageTable().GetPhysicalAddress(std::addressof(phys_addr), KProcessAddress(to_msg_buf));

            /* Convert the physical address to a linear pointer. */
            u32 *to_msg = GetPointer<u32>(KPageTable::GetHeapVirtualAddress(phys_addr));

            /* Set the error. */
            ams::svc::ipc::MessageBuffer msg(to_msg, to_msg_buf_size);
            msg.SetAsyncResult(result);
        }

    }

    void KServerSession::Destroy() {
        MESOSPHERE_ASSERT_THIS();

        this->parent->OnServerClosed();

        /* TODO: this->CleanupRequests(); */

        this->parent->Close();
    }

    Result KServerSession::OnRequest(KSessionRequest *request) {
        MESOSPHERE_UNIMPLEMENTED();
    }

    void KServerSession::OnClientClosed() {
        MESOSPHERE_ASSERT_THIS();

        KScopedLightLock lk(this->lock);

        /* Handle any pending requests. */
        KSessionRequest *prev_request = nullptr;
        while (true) {
            /* Declare variables for processing the request. */
            KSessionRequest *request = nullptr;
            KWritableEvent  *event   = nullptr;
            KThread         *thread  = nullptr;
            bool cur_request = false;
            bool terminate   = false;

            /* Get the next request. */
            {
                KScopedSchedulerLock sl;

                if (this->current_request != nullptr && this->current_request != prev_request) {
                    /* Set the request, open a reference as we process it. */
                    request = this->current_request;
                    request->Open();
                    cur_request = true;

                    /* Get thread and event for the request. */
                    thread = request->GetThread();
                    event  = request->GetEvent();

                    /* If the thread is terminating, handle that. */
                    if (thread->IsTerminationRequested()) {
                        request->ClearThread();
                        request->ClearEvent();
                        terminate = true;
                    }
                    prev_request = request;
                } else if (!this->request_list.empty()) {
                    /* Pop the request from the front of the list. */
                    request = std::addressof(this->request_list.front());
                    this->request_list.pop_front();

                    /* Get thread and event for the request. */
                    thread = request->GetThread();
                    event  = request->GetEvent();
                }
            }

            /* If there are no requests, we're done. */
            if (request == nullptr) {
                break;
            }

            /* All requests must have threads. */
            MESOSPHERE_ASSERT(thread != nullptr);

            /* Ensure that we close the request when done. */
            ON_SCOPE_EXIT { request->Close(); };

            /* If we're terminating, close a reference to the thread and event. */
            if (terminate) {
                thread->Close();
                if (event != nullptr) {
                    event->Close();
                }
            }

            /* If we need to, reply. */
            if (event != nullptr && !cur_request) {
                /* There must be no mappings. */
                MESOSPHERE_ASSERT(request->GetSendCount()     == 0);
                MESOSPHERE_ASSERT(request->GetReceiveCount()  == 0);
                MESOSPHERE_ASSERT(request->GetExchangeCount() == 0);

                /* Get the process and page table. */
                KProcess *client_process = thread->GetOwner();
                auto &client_pt = client_process->GetPageTable();

                /* Reply to the request. */
                ReplyAsyncError(client_process, request->GetAddress(), request->GetSize(), svc::ResultSessionClosed());

                /* Unlock the buffer. */
                /* NOTE: Nintendo does not check the result of this. */
                client_pt.UnlockForIpcUserBuffer(request->GetAddress(), request->GetSize());

                /* Signal the event. */
                event->Signal();
            }
        }

        /* Notify. */
        this->NotifyAbort(svc::ResultSessionClosed());
    }

}
