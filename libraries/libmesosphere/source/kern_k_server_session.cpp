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

    namespace ipc {

        using MessageBuffer = ams::svc::ipc::MessageBuffer;

    }

    namespace {

        class ReceiveList {
            private:
                u32 data[ipc::MessageBuffer::MessageHeader::ReceiveListCountType_CountMax * ipc::MessageBuffer::ReceiveListEntry::GetDataSize() / sizeof(u32)];
                s32 recv_list_count;
                uintptr_t msg_buffer_end;
                uintptr_t msg_buffer_space_end;
            public:
                static constexpr int GetEntryCount(const ipc::MessageBuffer::MessageHeader &header) {
                    const auto count = header.GetReceiveListCount();
                    switch (count) {
                        case ipc::MessageBuffer::MessageHeader::ReceiveListCountType_None:
                            return 0;
                        case ipc::MessageBuffer::MessageHeader::ReceiveListCountType_ToMessageBuffer:
                            return 0;
                        case ipc::MessageBuffer::MessageHeader::ReceiveListCountType_ToSingleBuffer:
                            return 1;
                        default:
                            return count - ipc::MessageBuffer::MessageHeader::ReceiveListCountType_CountOffset;
                    }
                }
            public:
                ReceiveList(const u32 *dst_msg, const ipc::MessageBuffer::MessageHeader &dst_header, const ipc::MessageBuffer::SpecialHeader &dst_special_header, size_t msg_size, size_t out_offset, s32 dst_recv_list_idx) {
                    this->recv_list_count      = dst_header.GetReceiveListCount();
                    this->msg_buffer_end       = reinterpret_cast<uintptr_t>(dst_msg) + sizeof(u32) * out_offset;
                    this->msg_buffer_space_end = reinterpret_cast<uintptr_t>(dst_msg) + msg_size;

                    const u32 *recv_list = dst_msg + dst_recv_list_idx;
                    __builtin_memcpy(this->data, recv_list, GetEntryCount(dst_header) * ipc::MessageBuffer::ReceiveListEntry::GetDataSize());
                }

                constexpr bool IsIndex() const {
                    return this->recv_list_count > ipc::MessageBuffer::MessageHeader::ReceiveListCountType_CountOffset;
                }
        };

        template<bool MoveHandleAllowed>
        ALWAYS_INLINE Result ProcessMessageSpecialData(int &offset, KProcess &dst_process, KProcess &src_process, KThread &src_thread, const ipc::MessageBuffer &dst_msg, const ipc::MessageBuffer &src_msg, const ipc::MessageBuffer::SpecialHeader &src_special_header) {
            /* Copy the special header to the destination. */
            offset = dst_msg.Set(src_special_header);

            /* Copy the process ID. */
            if (src_special_header.GetHasProcessId()) {
                /* TODO: Atmosphere mitm extension support. */
                offset = dst_msg.SetProcessId(offset, src_process.GetId());
            }

            /* Prepare to process handles. */
            auto &dst_handle_table = dst_process.GetHandleTable();
            auto &src_handle_table = src_process.GetHandleTable();
            Result result = ResultSuccess();

            /* Process copy handles. */
            for (auto i = 0; i < src_special_header.GetCopyHandleCount(); ++i) {
                /* Get the handles. */
                const ams::svc::Handle src_handle = src_msg.GetHandle(offset);
                      ams::svc::Handle dst_handle = ams::svc::InvalidHandle;

                /* If we're in a success state, try to move the handle to the new table. */
                if (R_SUCCEEDED(result) && src_handle != ams::svc::InvalidHandle) {
                    KScopedAutoObject obj = src_handle_table.GetObjectForIpc(src_handle, std::addressof(src_thread));
                    if (obj.IsNotNull()) {
                        Result add_result = dst_handle_table.Add(std::addressof(dst_handle), obj.GetPointerUnsafe());
                        if (R_FAILED(add_result)) {
                            result     = add_result;
                            dst_handle = ams::svc::InvalidHandle;
                        }
                    } else {
                        result = svc::ResultInvalidHandle();
                    }
                }

                /* Set the handle. */
                offset = dst_msg.SetHandle(offset, dst_handle);
            }

            /* Process move handles. */
            if constexpr (MoveHandleAllowed) {
                for (auto i = 0; i < src_special_header.GetMoveHandleCount(); ++i) {
                    /* Get the handles. */
                    const ams::svc::Handle src_handle = src_msg.GetHandle(offset);
                          ams::svc::Handle dst_handle = ams::svc::InvalidHandle;

                    /* Whether or not we've succeeded, we need to remove the handles from the source table. */
                    if (src_handle != ams::svc::InvalidHandle) {
                        if (R_SUCCEEDED(result)) {
                            KScopedAutoObject obj = src_handle_table.GetObjectForIpcWithoutPseudoHandle(src_handle);
                            if (obj.IsNotNull()) {
                                Result add_result = dst_handle_table.Add(std::addressof(dst_handle), obj.GetPointerUnsafe());

                                src_handle_table.Remove(src_handle);

                                if (R_FAILED(add_result)) {
                                    result     = add_result;
                                    dst_handle = ams::svc::InvalidHandle;
                                }
                            } else {
                                result = svc::ResultInvalidHandle();
                            }
                        } else {
                            src_handle_table.Remove(src_handle);
                        }
                    }

                    /* Set the handle. */
                    offset = dst_msg.SetHandle(offset, dst_handle);
                }
            }

            return result;
        }

        ALWAYS_INLINE Result ReceiveMessage(bool &recv_list_broken, uintptr_t dst_message_buffer, size_t dst_buffer_size, KPhysicalAddress dst_message_paddr, KThread &src_thread, uintptr_t src_message_buffer, size_t src_buffer_size, KServerSession *session, KSessionRequest *request) {
            /* Prepare variables for receive. */
            const KThread &dst_thread = GetCurrentThread();
            KProcess &dst_process     = *(dst_thread.GetOwnerProcess());
            KProcess &src_process     = *(src_thread.GetOwnerProcess());
            auto &dst_page_table      = dst_process.GetPageTable();
            auto &src_page_table      = src_process.GetPageTable();

            /* The receive list is initially not broken. */
            recv_list_broken = false;

            /* Set the server process for the request. */
            request->SetServerProcess(std::addressof(dst_process));

            /* Determine the message buffers. */
            u32 *dst_msg_ptr, *src_msg_ptr;
            bool dst_user, src_user;

            if (dst_message_buffer) {
                dst_msg_ptr = GetPointer<u32>(KPageTable::GetHeapVirtualAddress(dst_message_paddr));
                dst_user    = true;
            } else {
                dst_msg_ptr        = static_cast<ams::svc::ThreadLocalRegion *>(dst_thread.GetThreadLocalRegionHeapAddress())->message_buffer;
                dst_buffer_size    = sizeof(ams::svc::ThreadLocalRegion{}.message_buffer);
                dst_message_buffer = GetInteger(dst_thread.GetThreadLocalRegionAddress());
                dst_user           = false;
            }

            if (src_message_buffer) {
                /* NOTE: Nintendo does not check the result of this GetPhysicalAddress call. */
                KPhysicalAddress src_message_paddr;
                src_page_table.GetPhysicalAddress(std::addressof(src_message_paddr), src_message_buffer);

                src_msg_ptr = GetPointer<u32>(KPageTable::GetHeapVirtualAddress(src_message_paddr));
                src_user    = true;
            } else {
                src_msg_ptr        = static_cast<ams::svc::ThreadLocalRegion *>(src_thread.GetThreadLocalRegionHeapAddress())->message_buffer;
                src_buffer_size    = sizeof(ams::svc::ThreadLocalRegion{}.message_buffer);
                src_message_buffer = GetInteger(src_thread.GetThreadLocalRegionAddress());
                src_user           = false;
            }

            /* Parse the headers. */
            const ipc::MessageBuffer dst_msg(dst_msg_ptr, dst_buffer_size);
            const ipc::MessageBuffer src_msg(src_msg_ptr, src_buffer_size);
            const ipc::MessageBuffer::MessageHeader dst_header(dst_msg);
            const ipc::MessageBuffer::MessageHeader src_header(src_msg);
            const ipc::MessageBuffer::SpecialHeader dst_special_header(dst_msg, dst_header);
            const ipc::MessageBuffer::SpecialHeader src_special_header(src_msg, src_header);

            /* Get the end of the source message. */
            const size_t src_end_offset = ipc::MessageBuffer::GetRawDataIndex(src_header, src_special_header) + src_header.GetRawCount();

            /* Ensure that the headers fit. */
            R_UNLESS(ipc::MessageBuffer::GetMessageBufferSize(dst_header, dst_special_header) <= dst_buffer_size, svc::ResultInvalidCombination());
            R_UNLESS(ipc::MessageBuffer::GetMessageBufferSize(src_header, src_special_header) <= src_buffer_size, svc::ResultInvalidCombination());

            /* Ensure the receive list offset is after the end of raw data. */
            if (dst_header.GetReceiveListOffset()) {
                R_UNLESS(dst_header.GetReceiveListOffset() >= ipc::MessageBuffer::GetRawDataIndex(dst_header, dst_special_header) + dst_header.GetRawCount(), svc::ResultInvalidCombination());
            }

            /* Ensure that the destination buffer is big enough to receive the source. */
            R_UNLESS(dst_buffer_size >= src_end_offset * sizeof(u32), svc::ResultMessageTooLarge());

            /* Get the receive list. */
            const s32 dst_recv_list_idx = static_cast<s32>(ipc::MessageBuffer::GetReceiveListIndex(dst_header, dst_special_header));
            ReceiveList dst_recv_list(dst_msg_ptr, dst_header, dst_special_header, dst_buffer_size, src_end_offset, dst_recv_list_idx);

            /* Ensure that the source special header isn't invalid. */
            const bool src_has_special_header = src_header.GetHasSpecialHeader();
            if (src_has_special_header) {
                /* Sending move handles from client -> server is not allowed. */
                R_UNLESS(src_special_header.GetMoveHandleCount() == 0, svc::ResultInvalidCombination());
            }

            /* Prepare for further processing. */
            int pointer_key = 0;
            int offset = dst_msg.Set(src_header);

            /* Set up a guard to make sure that we end up in a clean state on error. */
            auto cleanup_guard = SCOPE_GUARD {
                /* TODO */
                MESOSPHERE_UNIMPLEMENTED();
            };

            /* Process any special data. */
            if (src_header.GetHasSpecialHeader()) {
                /* After we process, make sure we track whether the receive list is broken. */
                ON_SCOPE_EXIT { if (offset > dst_recv_list_idx) { recv_list_broken = true; } };

                /* Process special data. */
                R_TRY(ProcessMessageSpecialData<false>(offset, dst_process, src_process, src_thread, dst_msg, src_msg, src_special_header));
            }

            /* Process any pointer buffers. */
            for (auto i = 0; i < src_header.GetPointerCount(); ++i) {
                MESOSPHERE_UNIMPLEMENTED();
            }

            /* Process any map alias buffers. */
            for (auto i = 0; i < src_header.GetMapAliasCount(); ++i) {
                MESOSPHERE_UNIMPLEMENTED();
            }

            /* Process any raw data. */
            if (src_header.GetRawCount()) {
                MESOSPHERE_UNIMPLEMENTED();
            }

            /* TODO: Remove this when done, as these variables will be used by unimplemented stuff above. */
            static_cast<void>(dst_page_table);
            static_cast<void>(dst_user);
            static_cast<void>(src_user);
            static_cast<void>(pointer_key);

            /* We succeeded! */
            cleanup_guard.Cancel();
            return ResultSuccess();
        }

        ALWAYS_INLINE void ReplyAsyncError(KProcess *to_process, uintptr_t to_msg_buf, size_t to_msg_buf_size, Result result) {
            /* Convert the buffer to a physical address. */
            KPhysicalAddress phys_addr;
            to_process->GetPageTable().GetPhysicalAddress(std::addressof(phys_addr), KProcessAddress(to_msg_buf));

            /* Convert the physical address to a linear pointer. */
            u32 *to_msg = GetPointer<u32>(KPageTable::GetHeapVirtualAddress(phys_addr));

            /* Set the error. */
            ipc::MessageBuffer msg(to_msg, to_msg_buf_size);
            msg.SetAsyncResult(result);
        }

    }

    void KServerSession::Destroy() {
        MESOSPHERE_ASSERT_THIS();

        this->parent->OnServerClosed();

        /* TODO: this->CleanupRequests(); */

        this->parent->Close();
    }

    Result KServerSession::ReceiveRequest(uintptr_t server_message, uintptr_t server_buffer_size, KPhysicalAddress server_message_paddr) {
        MESOSPHERE_ASSERT_THIS();

        /* Lock the session. */
        KScopedLightLock lk(this->lock);

        /* Get the request and client thread. */
        KSessionRequest *request;
        KScopedAutoObject<KThread> client_thread;
        {
            KScopedSchedulerLock sl;

            /* Ensure that we can service the request. */
            R_UNLESS(!this->parent->IsClientClosed(), svc::ResultSessionClosed());

            /* Ensure we aren't already servicing a request. */
            R_UNLESS(this->current_request == nullptr, svc::ResultNotFound());

            /* Ensure we have a request to service. */
            R_UNLESS(!this->request_list.empty(), svc::ResultNotFound());

            /* Pop the first request from the list. */
            request = std::addressof(this->request_list.front());
            this->request_list.pop_front();

            /* Get the thread for the request. */
            client_thread = KScopedAutoObject<KThread>(request->GetThread());
            R_UNLESS(client_thread.IsNotNull(), svc::ResultSessionClosed());
        }

        /* Set the request as our current. */
        this->current_request = request;

        /* Get the client address. */
        uintptr_t client_message  = request->GetAddress();
        size_t client_buffer_size = request->GetSize();
        bool recv_list_broken = false;

        /* Receive the message. */
        Result result = ReceiveMessage(recv_list_broken, server_message, server_buffer_size, server_message_paddr, *client_thread.GetPointerUnsafe(), client_message, client_buffer_size, this, request);

        /* Handle cleanup on receive failure. */
        if (R_FAILED(result)) {
            /* TODO */
            MESOSPHERE_UNIMPLEMENTED();
        }

        return result;
    }

    Result KServerSession::SendReply(uintptr_t message, uintptr_t buffer_size, KPhysicalAddress message_paddr) {
        MESOSPHERE_UNIMPLEMENTED();
    }

    Result KServerSession::OnRequest(KSessionRequest *request) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Ensure that we can handle new requests. */
        R_UNLESS(!this->parent->IsServerClosed(), svc::ResultSessionClosed());

        /* If there's no event, this is synchronous, so we should check for thread termination. */
        if (request->GetEvent() == nullptr) {
            KThread *thread = request->GetThread();
            R_UNLESS(!thread->IsTerminationRequested(), svc::ResultTerminationRequested());
            thread->SetState(KThread::ThreadState_Waiting);
        }

        /* Get whether we're empty. */
        const bool was_empty = this->request_list.empty();

        /* Add the request to the list. */
        request->Open();
        this->request_list.push_back(*request);

        /* If we were empty, signal. */
        if (was_empty) {
            this->NotifyAvailable();
        }

        return ResultSuccess();
    }

    bool KServerSession::IsSignaledImpl() const {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* If the client is closed, we're always signaled. */
        if (this->parent->IsClientClosed()) {
            return true;
        }

        /* Otherwise, we're signaled if we have a request and aren't handling one. */
        return !this->request_list.empty() && this->current_request == nullptr;
    }

    bool KServerSession::IsSignaled() const {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        return this->IsSignaledImpl();
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
