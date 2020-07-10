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

        constexpr inline size_t PointerTransferBufferAlignment = 0x10;

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

                void GetBuffer(uintptr_t &out, size_t size, int &key) const {
                    switch (this->recv_list_count) {
                        case ipc::MessageBuffer::MessageHeader::ReceiveListCountType_None:
                            {
                                out = 0;
                            }
                            break;
                        case ipc::MessageBuffer::MessageHeader::ReceiveListCountType_ToMessageBuffer:
                            {
                                const uintptr_t buf = util::AlignUp(this->msg_buffer_end, PointerTransferBufferAlignment);

                                if ((buf < buf + size) && (buf + size <= this->msg_buffer_space_end)) {
                                    out = buf;
                                    key = buf + size - this->msg_buffer_end;
                                } else {
                                    out = 0;
                                }
                            }
                            break;
                        case ipc::MessageBuffer::MessageHeader::ReceiveListCountType_ToSingleBuffer:
                            {
                                const ipc::MessageBuffer::ReceiveListEntry entry(this->data[0], this->data[1]);
                                const uintptr_t buf = util::AlignUp(entry.GetAddress() + key, PointerTransferBufferAlignment);

                                const uintptr_t entry_addr = entry.GetAddress();
                                const size_t entry_size    = entry.GetSize();

                                if ((buf < buf + size) && (entry_addr < entry_addr + entry_size) && (buf + size <= entry_addr + entry_size)) {
                                    out = buf;
                                    key = buf + size - entry_addr;
                                } else {
                                    out = 0;
                                }
                            }
                            break;
                        default:
                            {
                                if (key < this->recv_list_count - ipc::MessageBuffer::MessageHeader::ReceiveListCountType_CountOffset) {
                                    const ipc::MessageBuffer::ReceiveListEntry entry(this->data[2 * key + 0], this->data[2 * key + 1]);

                                    const uintptr_t entry_addr = entry.GetAddress();
                                    const size_t entry_size    = entry.GetSize();

                                    if ((entry_addr < entry_addr + entry_size) && (entry_size >= size)) {
                                        out = entry_addr;
                                    }
                                } else {
                                    out = 0;
                                }
                            }
                            break;
                    }
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

        ALWAYS_INLINE Result ProcessReceiveMessagePointerDescriptors(int &offset, int &pointer_key, KProcessPageTable &dst_page_table, KProcessPageTable &src_page_table, const ipc::MessageBuffer &dst_msg, const ipc::MessageBuffer &src_msg, const ReceiveList &dst_recv_list, bool dst_user) {
            /* Get the offset at the start of processing. */
            const int cur_offset = offset;

            /* Get the pointer desc. */
            ipc::MessageBuffer::PointerDescriptor src_desc(src_msg, cur_offset);
            offset += ipc::MessageBuffer::PointerDescriptor::GetDataSize() / sizeof(u32);

            /* Extract address/size. */
            const uintptr_t src_pointer = src_desc.GetAddress();
            const size_t recv_size      = src_desc.GetSize();
            uintptr_t recv_pointer      = 0;

            /* Process the buffer, if it has a size. */
            if (recv_size > 0) {
                /* If using indexing, set index. */
                if (dst_recv_list.IsIndex()) {
                    pointer_key = src_desc.GetIndex();
                }

                /* Get the buffer. */
                dst_recv_list.GetBuffer(recv_pointer, recv_size, pointer_key);
                R_UNLESS(recv_pointer != 0, svc::ResultOutOfResource());

                /* Perform the pointer data copy. */
                if (dst_user) {
                    R_TRY(src_page_table.CopyMemoryFromLinearToLinearWithoutCheckDestination(dst_page_table, recv_pointer, recv_size,
                                                                                             KMemoryState_FlagReferenceCounted, KMemoryState_FlagReferenceCounted,
                                                                                             static_cast<KMemoryPermission>(KMemoryPermission_NotMapped | KMemoryPermission_KernelReadWrite),
                                                                                             KMemoryAttribute_AnyLocked | KMemoryAttribute_Uncached | KMemoryAttribute_Locked, KMemoryAttribute_AnyLocked | KMemoryAttribute_Locked,
                                                                                             src_pointer,
                                                                                             KMemoryState_FlagReferenceCounted, KMemoryState_FlagReferenceCounted,
                                                                                             KMemoryPermission_UserRead,
                                                                                             KMemoryAttribute_Uncached, KMemoryAttribute_None));
                } else {
                    R_TRY(src_page_table.CopyMemoryFromLinearToUser(recv_pointer, recv_size, src_pointer,
                                                                    KMemoryState_FlagReferenceCounted, KMemoryState_FlagReferenceCounted,
                                                                    KMemoryPermission_UserRead,
                                                                    KMemoryAttribute_Uncached, KMemoryAttribute_None));
                }
            }

            /* Set the output descriptor. */
            dst_msg.Set(cur_offset, ipc::MessageBuffer::PointerDescriptor(reinterpret_cast<void *>(recv_pointer), recv_size, src_desc.GetIndex()));

            return ResultSuccess();
        }

        constexpr ALWAYS_INLINE Result GetMapAliasMemoryState(KMemoryState &out, ipc::MessageBuffer::MapAliasDescriptor::Attribute attr) {
            switch (attr) {
                case ipc::MessageBuffer::MapAliasDescriptor::Attribute_Ipc:          out = KMemoryState_Ipc;          break;
                case ipc::MessageBuffer::MapAliasDescriptor::Attribute_NonSecureIpc: out = KMemoryState_NonSecureIpc; break;
                case ipc::MessageBuffer::MapAliasDescriptor::Attribute_NonDeviceIpc: out = KMemoryState_NonDeviceIpc; break;
                default: return svc::ResultInvalidCombination();
            }

            return ResultSuccess();
        }

        ALWAYS_INLINE Result ProcessReceiveMessageMapAliasDescriptors(int &offset, KProcessPageTable &dst_page_table, KProcessPageTable &src_page_table, const ipc::MessageBuffer &dst_msg, const ipc::MessageBuffer &src_msg, KSessionRequest *request, KMemoryPermission perm, bool send) {
            /* Get the offset at the start of processing. */
            const int cur_offset = offset;

            /* Get the map alias descriptor. */
            ipc::MessageBuffer::MapAliasDescriptor src_desc(src_msg, cur_offset);
            offset += ipc::MessageBuffer::MapAliasDescriptor::GetDataSize() / sizeof(u32);

            /* Extract address/size. */
            const KProcessAddress src_address = src_desc.GetAddress();
            const size_t size                 = src_desc.GetSize();
            KProcessAddress dst_address       = 0;

            /* Determine the result memory state. */
            KMemoryState dst_state;
            R_TRY(GetMapAliasMemoryState(dst_state, src_desc.GetAttribute()));

            /* Process the buffer, if it has a size. */
            if (size > 0) {
                /* Set up the source pages for ipc. */
                R_TRY(dst_page_table.SetupForIpc(std::addressof(dst_address), size, src_address, src_page_table, perm, dst_state, send));

                /* Ensure that we clean up on failure. */
                auto setup_guard = SCOPE_GUARD {
                    dst_page_table.CleanupForIpcServer(dst_address, size, dst_state, request->GetServerProcess());
                    src_page_table.CleanupForIpcClient(src_address, size, dst_state);
                };

                /* Push the appropriate mapping. */
                if (perm == KMemoryPermission_UserRead) {
                    R_TRY(request->PushSend(src_address, dst_address, size, dst_state));
                } else if (send) {
                    R_TRY(request->PushExchange(src_address, dst_address, size, dst_state));
                } else {
                    R_TRY(request->PushReceive(src_address, dst_address, size, dst_state));
                }

                /* We successfully pushed the mapping. */
                setup_guard.Cancel();
            }

            /* Set the output descriptor. */
            dst_msg.Set(cur_offset, ipc::MessageBuffer::MapAliasDescriptor(GetVoidPointer(dst_address), size, src_desc.GetAttribute()));

            return ResultSuccess();
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
                /* After we process, make sure we track whether the receive list is broken. */
                ON_SCOPE_EXIT { if (offset > dst_recv_list_idx) { recv_list_broken = true; } };

                R_TRY(ProcessReceiveMessagePointerDescriptors(offset, pointer_key, dst_page_table, src_page_table, dst_msg, src_msg, dst_recv_list, dst_user && dst_header.GetReceiveListCount() == ipc::MessageBuffer::MessageHeader::ReceiveListCountType_ToMessageBuffer));
            }

            /* Process any map alias buffers. */
            for (auto i = 0; i < src_header.GetMapAliasCount(); ++i) {
                /* After we process, make sure we track whether the receive list is broken. */
                ON_SCOPE_EXIT { if (offset > dst_recv_list_idx) { recv_list_broken = true; } };

                /* We process in order send, recv, exch. Buffers after send (recv/exch) are ReadWrite. */
                const KMemoryPermission perm = (i >= src_header.GetSendCount()) ? KMemoryPermission_UserReadWrite : KMemoryPermission_UserRead;

                /* Buffer is send if it is send or exch. */
                const bool send = (i < src_header.GetSendCount()) || (i >= src_header.GetSendCount() + src_header.GetReceiveCount());

                R_TRY(ProcessReceiveMessageMapAliasDescriptors(offset, dst_page_table, src_page_table, dst_msg, src_msg, request, perm, send));
            }

            /* Process any raw data. */
            if (const auto raw_count = src_header.GetRawCount(); raw_count != 0) {
                /* After we process, make sure we track whether the receive list is broken. */
                ON_SCOPE_EXIT { if (offset + raw_count > dst_recv_list_idx) { recv_list_broken = true; } };

                /* Get the offset and size. */
                const size_t offset_words = offset * sizeof(u32);
                const size_t raw_size     = raw_count * sizeof(u32);

                /* Fast case is TLS -> TLS, do raw memcpy if we can. */
                if (!dst_user && !src_user) {
                    std::memcpy(dst_msg_ptr + offset, src_msg_ptr + offset, raw_size);
                } else if (dst_user) {
                    /* Determine how much fast size we can copy. */
                    const size_t max_fast_size = std::min<size_t>(offset_words + raw_size, PageSize);
                    const size_t fast_size     = max_fast_size - offset_words;

                    /* Determine the source permission. User buffer should be unmapped + read, TLS should be user readable. */
                    const KMemoryPermission src_perm = static_cast<KMemoryPermission>(src_user ? KMemoryPermission_NotMapped | KMemoryPermission_KernelRead : KMemoryPermission_UserRead);

                    /* Perform the fast part of the copy. */
                    R_TRY(src_page_table.CopyMemoryFromLinearToKernel(reinterpret_cast<uintptr_t>(dst_msg_ptr) + offset_words, fast_size, src_message_buffer + offset_words,
                                                                      KMemoryState_FlagReferenceCounted, KMemoryState_FlagReferenceCounted,
                                                                      src_perm,
                                                                      KMemoryAttribute_Uncached, KMemoryAttribute_None));

                    /* If the fast part of the copy didn't get everything, perform the slow part of the copy. */
                    if (fast_size < raw_size) {
                        R_TRY(src_page_table.CopyMemoryFromLinearToLinear(dst_page_table, dst_message_buffer + max_fast_size, raw_size - fast_size,
                                                                          KMemoryState_FlagReferenceCounted, KMemoryState_FlagReferenceCounted,
                                                                          static_cast<KMemoryPermission>(KMemoryPermission_NotMapped | KMemoryPermission_KernelReadWrite),
                                                                          KMemoryAttribute_AnyLocked | KMemoryAttribute_Uncached | KMemoryAttribute_Locked, KMemoryAttribute_AnyLocked | KMemoryAttribute_Locked,
                                                                          src_message_buffer + max_fast_size,
                                                                          KMemoryState_FlagReferenceCounted, KMemoryState_FlagReferenceCounted,
                                                                          src_perm,
                                                                          KMemoryAttribute_Uncached, KMemoryAttribute_None));
                    }
                } else /* if (src_user) */ {
                    /* The source is a user buffer, so it should be unmapped + readable. */
                    constexpr KMemoryPermission SourcePermission = static_cast<KMemoryPermission>(KMemoryPermission_NotMapped | KMemoryPermission_KernelRead);

                    /* Copy the memory. */
                    R_TRY(src_page_table.CopyMemoryFromLinearToUser(dst_message_buffer + offset_words, raw_size, src_message_buffer + offset_words,
                                                                    KMemoryState_FlagReferenceCounted, KMemoryState_FlagReferenceCounted,
                                                                    SourcePermission,
                                                                    KMemoryAttribute_Uncached, KMemoryAttribute_None));
                }
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
