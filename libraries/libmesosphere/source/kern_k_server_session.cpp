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

    #pragma GCC push_options
    #pragma GCC optimize ("-O3")

    namespace ipc {

        using MessageBuffer = ams::svc::ipc::MessageBuffer;

    }

    namespace {

        constexpr inline size_t PointerTransferBufferAlignment = 0x10;

        class ReceiveList {
            private:
                u32 m_data[ipc::MessageBuffer::MessageHeader::ReceiveListCountType_CountMax * ipc::MessageBuffer::ReceiveListEntry::GetDataSize() / sizeof(u32)];
                s32 m_recv_list_count;
                uintptr_t m_msg_buffer_end;
                uintptr_t m_msg_buffer_space_end;
            public:
                static constexpr ALWAYS_INLINE int GetEntryCount(const ipc::MessageBuffer::MessageHeader &header) {
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
                ReceiveList(const u32 *dst_msg, uintptr_t dst_address, const KProcessPageTable &dst_page_table, const ipc::MessageBuffer::MessageHeader &dst_header, const ipc::MessageBuffer::SpecialHeader &dst_special_header, size_t msg_size, size_t out_offset, s32 dst_recv_list_idx, bool is_tls) {
                    m_recv_list_count      = dst_header.GetReceiveListCount();
                    m_msg_buffer_end       = dst_address + sizeof(u32) * out_offset;
                    m_msg_buffer_space_end = dst_address + msg_size;

                    /* NOTE: Nintendo calculates the receive list index here using the special header. */
                    /* We pre-calculate it in the caller, and pass it as a parameter. */
                    MESOSPHERE_UNUSED(dst_special_header);

                    const u32 *recv_list   = dst_msg + dst_recv_list_idx;
                    const auto entry_count = GetEntryCount(dst_header);

                    if (is_tls) {
                        __builtin_memcpy(m_data, recv_list, entry_count * ipc::MessageBuffer::ReceiveListEntry::GetDataSize());
                    } else {
                        uintptr_t page_addr = util::AlignDown(dst_address, PageSize);
                        uintptr_t cur_addr  = dst_address + dst_recv_list_idx * sizeof(u32);
                        for (size_t i = 0; i < entry_count * ipc::MessageBuffer::ReceiveListEntry::GetDataSize() / sizeof(u32); ++i) {
                            if (page_addr != util::AlignDown(cur_addr, PageSize)) {
                                KPhysicalAddress phys_addr;
                                dst_page_table.GetPhysicalAddress(std::addressof(phys_addr), KProcessAddress(cur_addr));

                                recv_list = GetPointer<u32>(KPageTable::GetHeapVirtualAddress(phys_addr));
                                page_addr = util::AlignDown(cur_addr, PageSize);
                            }
                            m_data[i] = *(recv_list++);
                            cur_addr += sizeof(u32);
                        }
                    }
                }

                constexpr ALWAYS_INLINE bool IsIndex() const {
                    return m_recv_list_count > ipc::MessageBuffer::MessageHeader::ReceiveListCountType_CountOffset;
                }

                void GetBuffer(uintptr_t &out, size_t size, int &key) const {
                    switch (m_recv_list_count) {
                        case ipc::MessageBuffer::MessageHeader::ReceiveListCountType_None:
                            {
                                out = 0;
                            }
                            break;
                        case ipc::MessageBuffer::MessageHeader::ReceiveListCountType_ToMessageBuffer:
                            {
                                const uintptr_t buf = util::AlignUp(m_msg_buffer_end + key, PointerTransferBufferAlignment);

                                if ((buf < buf + size) && (buf + size <= m_msg_buffer_space_end)) {
                                    out = buf;
                                    key = buf + size - m_msg_buffer_end;
                                } else {
                                    out = 0;
                                }
                            }
                            break;
                        case ipc::MessageBuffer::MessageHeader::ReceiveListCountType_ToSingleBuffer:
                            {
                                const ipc::MessageBuffer::ReceiveListEntry entry(m_data[0], m_data[1]);
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
                                if (key < m_recv_list_count - ipc::MessageBuffer::MessageHeader::ReceiveListCountType_CountOffset) {
                                    const ipc::MessageBuffer::ReceiveListEntry entry(m_data[2 * key + 0], m_data[2 * key + 1]);

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
                /* NOTE: Atmosphere extends the official kernel here to enable the mitm api. */
                /* If building the kernel without this support, just set the following to false. */
                constexpr bool EnableProcessIdPassthroughForAtmosphere = true;

                if constexpr (EnableProcessIdPassthroughForAtmosphere) {
                    constexpr u64 PassthroughProcessIdMask  = UINT64_C(0xFFFF000000000000);
                    constexpr u64 PassthroughProcessIdValue = UINT64_C(0xFFFE000000000000);
                    static_assert((PassthroughProcessIdMask & PassthroughProcessIdValue) == PassthroughProcessIdValue);

                    const u64 src_process_id_value = src_msg.GetProcessId(offset);
                    const bool is_passthrough = (src_process_id_value & PassthroughProcessIdMask) == PassthroughProcessIdValue;

                    offset = dst_msg.SetProcessId(offset, is_passthrough ? (src_process_id_value & ~PassthroughProcessIdMask) : src_process.GetId());
                } else {
                    offset = dst_msg.SetProcessId(offset, src_process.GetId());
                }
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
                    R_TRY(src_page_table.CopyMemoryFromHeapToHeapWithoutCheckDestination(dst_page_table, recv_pointer, recv_size,
                                                                                         KMemoryState_FlagReferenceCounted, KMemoryState_FlagReferenceCounted,
                                                                                         static_cast<KMemoryPermission>(KMemoryPermission_NotMapped | KMemoryPermission_KernelReadWrite),
                                                                                         KMemoryAttribute_Uncached | KMemoryAttribute_Locked, KMemoryAttribute_Locked,
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

        constexpr ALWAYS_INLINE Result GetMapAliasTestStateAndAttributeMask(u32 &out_state, u32 &out_attr_mask, KMemoryState state) {
            switch (state) {
                case KMemoryState_Ipc:
                    out_state     = KMemoryState_FlagCanUseIpc;
                    out_attr_mask = KMemoryAttribute_Uncached | KMemoryAttribute_DeviceShared | KMemoryAttribute_Locked;
                    break;
                case KMemoryState_NonSecureIpc:
                    out_state     = KMemoryState_FlagCanUseNonSecureIpc;
                    out_attr_mask = KMemoryAttribute_Uncached | KMemoryAttribute_Locked;
                    break;
                case KMemoryState_NonDeviceIpc:
                    out_state     = KMemoryState_FlagCanUseNonDeviceIpc;
                    out_attr_mask = KMemoryAttribute_Uncached | KMemoryAttribute_Locked;
                    break;
                default:
                    return svc::ResultInvalidCombination();
            }

            return ResultSuccess();
        }

        ALWAYS_INLINE void CleanupSpecialData(KProcess &dst_process, u32 *dst_msg_ptr, size_t dst_buffer_size) {
            /* Parse the message. */
            const ipc::MessageBuffer dst_msg(dst_msg_ptr, dst_buffer_size);
            const ipc::MessageBuffer::MessageHeader dst_header(dst_msg);
            const ipc::MessageBuffer::SpecialHeader dst_special_header(dst_msg, dst_header);

            /* Check that the size is big enough. */
            if (ipc::MessageBuffer::GetMessageBufferSize(dst_header, dst_special_header) > dst_buffer_size) {
                return;
            }

            /* Set the special header. */
            int offset = dst_msg.Set(dst_special_header);

            /* Clear the process id, if needed. */
            if (dst_special_header.GetHasProcessId()) {
                offset = dst_msg.SetProcessId(offset, 0);
            }

            /* Clear handles, as relevant. */
            auto &dst_handle_table = dst_process.GetHandleTable();
            for (auto i = 0; i < (dst_special_header.GetCopyHandleCount() + dst_special_header.GetMoveHandleCount()); ++i) {
                const ams::svc::Handle handle = dst_msg.GetHandle(offset);

                if (handle != ams::svc::InvalidHandle) {
                    dst_handle_table.Remove(handle);
                }

                offset = dst_msg.SetHandle(offset, ams::svc::InvalidHandle);
            }
        }

        ALWAYS_INLINE Result CleanupServerHandles(uintptr_t message, size_t buffer_size, KPhysicalAddress message_paddr) {
            /* Server is assumed to be current thread. */
            const KThread &thread = GetCurrentThread();

            /* Get the linear message pointer. */
            u32 *msg_ptr;
            if (message) {
                msg_ptr = GetPointer<u32>(KPageTable::GetHeapVirtualAddress(message_paddr));
            } else {
                msg_ptr     = static_cast<ams::svc::ThreadLocalRegion *>(thread.GetThreadLocalRegionHeapAddress())->message_buffer;
                buffer_size = sizeof(ams::svc::ThreadLocalRegion{}.message_buffer);
                message     = GetInteger(thread.GetThreadLocalRegionAddress());
            }

            /* Parse the message. */
            const ipc::MessageBuffer msg(msg_ptr, buffer_size);
            const ipc::MessageBuffer::MessageHeader header(msg);
            const ipc::MessageBuffer::SpecialHeader special_header(msg, header);

            /* Check that the size is big enough. */
            R_UNLESS(ipc::MessageBuffer::GetMessageBufferSize(header, special_header) <= buffer_size, svc::ResultInvalidCombination());

            /* If there's a special header, there may be move handles we need to close. */
            if (header.GetHasSpecialHeader()) {
                /* Determine the offset to the start of handles. */
                auto offset = msg.GetSpecialDataIndex(header, special_header);
                if (special_header.GetHasProcessId()) {
                    offset += sizeof(u64) / sizeof(u32);
                }
                if (auto copy_count = special_header.GetCopyHandleCount(); copy_count > 0) {
                    offset += (sizeof(ams::svc::Handle) * copy_count) / sizeof(u32);
                }

                /* Get the handle table. */
                auto &handle_table = thread.GetOwnerProcess()->GetHandleTable();

                /* Close the handles. */
                for (auto i = 0; i < special_header.GetMoveHandleCount(); ++i) {
                    handle_table.Remove(msg.GetHandle(offset));
                    offset += sizeof(ams::svc::Handle) / sizeof(u32);
                }
            }

            return ResultSuccess();
        }

        ALWAYS_INLINE Result CleanupServerMap(KSessionRequest *request, KProcess *server_process) {
            /* If there's no server process, there's nothing to clean up. */
            R_SUCCEED_IF(server_process == nullptr);

            /* Get the page table. */
            auto &server_page_table = server_process->GetPageTable();

            /* Cleanup Send mappings. */
            for (size_t i = 0; i < request->GetSendCount(); ++i) {
                R_TRY(server_page_table.CleanupForIpcServer(request->GetSendServerAddress(i), request->GetSendSize(i), request->GetSendMemoryState(i), server_process));
            }

            /* Cleanup Receive mappings. */
            for (size_t i = 0; i < request->GetReceiveCount(); ++i) {
                R_TRY(server_page_table.CleanupForIpcServer(request->GetReceiveServerAddress(i), request->GetReceiveSize(i), request->GetReceiveMemoryState(i), server_process));
            }

            /* Cleanup Exchange mappings. */
            for (size_t i = 0; i < request->GetExchangeCount(); ++i) {
                R_TRY(server_page_table.CleanupForIpcServer(request->GetExchangeServerAddress(i), request->GetExchangeSize(i), request->GetExchangeMemoryState(i), server_process));
            }

            return ResultSuccess();
        }

        ALWAYS_INLINE Result CleanupClientMap(KSessionRequest *request, KProcessPageTable *client_page_table) {
            /* If there's no client page table, there's nothing to clean up. */
            R_SUCCEED_IF(client_page_table == nullptr);

            /* Cleanup Send mappings. */
            for (size_t i = 0; i < request->GetSendCount(); ++i) {
                R_TRY(client_page_table->CleanupForIpcClient(request->GetSendClientAddress(i), request->GetSendSize(i), request->GetSendMemoryState(i)));
            }

            /* Cleanup Receive mappings. */
            for (size_t i = 0; i < request->GetReceiveCount(); ++i) {
                R_TRY(client_page_table->CleanupForIpcClient(request->GetReceiveClientAddress(i), request->GetReceiveSize(i), request->GetReceiveMemoryState(i)));
            }

            /* Cleanup Exchange mappings. */
            for (size_t i = 0; i < request->GetExchangeCount(); ++i) {
                R_TRY(client_page_table->CleanupForIpcClient(request->GetExchangeClientAddress(i), request->GetExchangeSize(i), request->GetExchangeMemoryState(i)));
            }

            return ResultSuccess();
        }

        ALWAYS_INLINE Result CleanupMap(KSessionRequest *request, KProcess *server_process, KProcessPageTable *client_page_table) {
            /* Cleanup the server map. */
            R_TRY(CleanupServerMap(request, server_process));

            /* Cleanup the client map. */
            R_TRY(CleanupClientMap(request, client_page_table));

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

            /* NOTE: Session is used only for debugging, and so may go unused. */
            MESOSPHERE_UNUSED(session);

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
            const s32 dst_recv_list_idx = ipc::MessageBuffer::GetReceiveListIndex(dst_header, dst_special_header);
            ReceiveList dst_recv_list(dst_msg_ptr, dst_message_buffer, dst_page_table, dst_header, dst_special_header, dst_buffer_size, src_end_offset, dst_recv_list_idx, !dst_user);

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
                /* Cleanup mappings. */
                CleanupMap(request, std::addressof(dst_process), std::addressof(src_page_table));

                /* Cleanup special data. */
                if (src_header.GetHasSpecialHeader()) {
                    CleanupSpecialData(dst_process, dst_msg_ptr, dst_buffer_size);
                }

                /* Cleanup the header if the receive list isn't broken. */
                if (!recv_list_broken) {
                    dst_msg.Set(dst_header);
                    if (dst_header.GetHasSpecialHeader()) {
                        dst_msg.Set(dst_special_header);
                    }
                }
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
                        R_TRY(src_page_table.CopyMemoryFromHeapToHeap(dst_page_table, dst_message_buffer + max_fast_size, raw_size - fast_size,
                                                                      KMemoryState_FlagReferenceCounted, KMemoryState_FlagReferenceCounted,
                                                                      static_cast<KMemoryPermission>(KMemoryPermission_NotMapped | KMemoryPermission_KernelReadWrite),
                                                                      KMemoryAttribute_Uncached | KMemoryAttribute_Locked, KMemoryAttribute_Locked,
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

            /* We succeeded! */
            cleanup_guard.Cancel();
            return ResultSuccess();
        }

        ALWAYS_INLINE Result ProcessSendMessageReceiveMapping(KProcessPageTable &dst_page_table, KProcessAddress client_address, KProcessAddress server_address, size_t size, KMemoryState src_state) {
            /* If the size is zero, there's nothing to process. */
            R_SUCCEED_IF(size == 0);

            /* Get the memory state and attribute mask to test. */
            u32 test_state;
            u32 test_attr_mask;
            R_TRY(GetMapAliasTestStateAndAttributeMask(test_state, test_attr_mask, src_state));

            /* Determine buffer extents. */
            KProcessAddress aligned_dst_start = util::AlignDown(GetInteger(client_address), PageSize);
            KProcessAddress aligned_dst_end   = util::AlignUp(GetInteger(client_address) + size, PageSize);
            KProcessAddress mapping_dst_start = util::AlignUp(GetInteger(client_address), PageSize);
            KProcessAddress mapping_dst_end   = util::AlignDown(GetInteger(client_address) + size, PageSize);

            KProcessAddress mapping_src_end   = util::AlignDown(GetInteger(server_address) + size, PageSize);

            /* If the start of the buffer is unaligned, handle that. */
            if (aligned_dst_start != mapping_dst_start) {
                MESOSPHERE_ASSERT(client_address < mapping_dst_start);
                const size_t copy_size = std::min<size_t>(size, mapping_dst_start - client_address);
                R_TRY(dst_page_table.CopyMemoryFromUserToLinear(client_address, copy_size,
                                                                test_state, test_state,
                                                                KMemoryPermission_UserReadWrite,
                                                                test_attr_mask, KMemoryAttribute_None,
                                                                server_address));
            }

            /* If the end of the buffer is unaligned, handle that. */
            if (mapping_dst_end < aligned_dst_end && (aligned_dst_start == mapping_dst_start || aligned_dst_start < mapping_dst_end)) {
                const size_t copy_size = client_address + size - mapping_dst_end;
                R_TRY(dst_page_table.CopyMemoryFromUserToLinear(mapping_dst_end, copy_size,
                                                                test_state, test_state,
                                                                KMemoryPermission_UserReadWrite,
                                                                test_attr_mask, KMemoryAttribute_None,
                                                                mapping_src_end));
            }

            return ResultSuccess();
        }

        ALWAYS_INLINE Result ProcessSendMessagePointerDescriptors(int &offset, int &pointer_key, KProcessPageTable &dst_page_table, const ipc::MessageBuffer &dst_msg, const ipc::MessageBuffer &src_msg, const ReceiveList &dst_recv_list, bool dst_user) {
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
                const KMemoryPermission dst_perm = static_cast<KMemoryPermission>(dst_user ? KMemoryPermission_NotMapped | KMemoryPermission_KernelReadWrite : KMemoryPermission_UserReadWrite);
                R_TRY(dst_page_table.CopyMemoryFromUserToLinear(recv_pointer, recv_size,
                                                                KMemoryState_FlagReferenceCounted, KMemoryState_FlagReferenceCounted,
                                                                dst_perm,
                                                                KMemoryAttribute_Uncached, KMemoryAttribute_None,
                                                                src_pointer));
            }

            /* Set the output descriptor. */
            dst_msg.Set(cur_offset, ipc::MessageBuffer::PointerDescriptor(reinterpret_cast<void *>(recv_pointer), recv_size, src_desc.GetIndex()));

            return ResultSuccess();
        }

        ALWAYS_INLINE Result SendMessage(uintptr_t src_message_buffer, size_t src_buffer_size, KPhysicalAddress src_message_paddr, KThread &dst_thread, uintptr_t dst_message_buffer, size_t dst_buffer_size, KServerSession *session, KSessionRequest *request) {
            /* Prepare variables for send. */
            KThread &src_thread   = GetCurrentThread();
            KProcess &dst_process = *(dst_thread.GetOwnerProcess());
            KProcess &src_process = *(src_thread.GetOwnerProcess());
            auto &dst_page_table  = dst_process.GetPageTable();
            auto &src_page_table  = src_process.GetPageTable();

            /* NOTE: Session is used only for debugging, and so may go unused. */
            MESOSPHERE_UNUSED(session);

            /* Determine the message buffers. */
            u32 *dst_msg_ptr, *src_msg_ptr;
            bool dst_user, src_user;

            if (dst_message_buffer) {
                /* NOTE: Nintendo does not check the result of this GetPhysicalAddress call. */
                KPhysicalAddress dst_message_paddr;
                dst_page_table.GetPhysicalAddress(std::addressof(dst_message_paddr), dst_message_buffer);

                dst_msg_ptr = GetPointer<u32>(KPageTable::GetHeapVirtualAddress(dst_message_paddr));
                dst_user    = true;
            } else {
                dst_msg_ptr        = static_cast<ams::svc::ThreadLocalRegion *>(dst_thread.GetThreadLocalRegionHeapAddress())->message_buffer;
                dst_buffer_size    = sizeof(ams::svc::ThreadLocalRegion{}.message_buffer);
                dst_message_buffer = GetInteger(dst_thread.GetThreadLocalRegionAddress());
                dst_user           = false;
            }

            if (src_message_buffer) {
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

            /* Declare variables for processing. */
            int offset = 0;
            int pointer_key = 0;
            bool processed_special_data = false;

            /* Set up a guard to make sure that we end up in a clean state on error. */
            auto cleanup_guard = SCOPE_GUARD {
                /* Cleanup special data. */
                if (processed_special_data) {
                    if (src_header.GetHasSpecialHeader()) {
                        CleanupSpecialData(dst_process, dst_msg_ptr, dst_buffer_size);
                    }
                } else {
                    CleanupServerHandles(src_user ? src_message_buffer : 0, src_buffer_size, src_message_paddr);
                }

                /* Cleanup mappings. */
                CleanupMap(request, std::addressof(src_process), std::addressof(dst_page_table));
            };

            /* Ensure that the headers fit. */
            R_UNLESS(ipc::MessageBuffer::GetMessageBufferSize(src_header, src_special_header) <= src_buffer_size, svc::ResultInvalidCombination());
            R_UNLESS(ipc::MessageBuffer::GetMessageBufferSize(dst_header, dst_special_header) <= dst_buffer_size, svc::ResultInvalidCombination());

            /* Ensure the receive list offset is after the end of raw data. */
            if (dst_header.GetReceiveListOffset()) {
                R_UNLESS(dst_header.GetReceiveListOffset() >= ipc::MessageBuffer::GetRawDataIndex(dst_header, dst_special_header) + dst_header.GetRawCount(), svc::ResultInvalidCombination());
            }

            /* Ensure that the destination buffer is big enough to receive the source. */
            R_UNLESS(dst_buffer_size >= src_end_offset * sizeof(u32), svc::ResultMessageTooLarge());

            /* Replies must have no buffers. */
            R_UNLESS(src_header.GetSendCount() == 0,     svc::ResultInvalidCombination());
            R_UNLESS(src_header.GetReceiveCount() == 0,  svc::ResultInvalidCombination());
            R_UNLESS(src_header.GetExchangeCount() == 0, svc::ResultInvalidCombination());

            /* Get the receive list. */
            const s32 dst_recv_list_idx = ipc::MessageBuffer::GetReceiveListIndex(dst_header, dst_special_header);
            ReceiveList dst_recv_list(dst_msg_ptr, dst_message_buffer, dst_page_table, dst_header, dst_special_header, dst_buffer_size, src_end_offset, dst_recv_list_idx, !dst_user);

            /* Handle any receive buffers. */
            for (size_t i = 0; i < request->GetReceiveCount(); ++i) {
                R_TRY(ProcessSendMessageReceiveMapping(dst_page_table, request->GetReceiveClientAddress(i), request->GetReceiveServerAddress(i), request->GetReceiveSize(i), request->GetReceiveMemoryState(i)));
            }

            /* Handle any exchange buffers. */
            for (size_t i = 0; i < request->GetExchangeCount(); ++i) {
                R_TRY(ProcessSendMessageReceiveMapping(dst_page_table, request->GetExchangeClientAddress(i), request->GetExchangeServerAddress(i), request->GetExchangeSize(i), request->GetExchangeMemoryState(i)));
            }

            /* Set the header. */
            offset = dst_msg.Set(src_header);

            /* Process any special data. */
            MESOSPHERE_ASSERT(GetCurrentThreadPointer() == std::addressof(src_thread));
            processed_special_data = true;
            if (src_header.GetHasSpecialHeader()) {
                R_TRY(ProcessMessageSpecialData<true>(offset, dst_process, src_process, src_thread, dst_msg, src_msg, src_special_header));
            }

            /* Process any pointer buffers. */
            for (auto i = 0; i < src_header.GetPointerCount(); ++i) {
                R_TRY(ProcessSendMessagePointerDescriptors(offset, pointer_key, dst_page_table, dst_msg, src_msg, dst_recv_list, dst_user && dst_header.GetReceiveListCount() == ipc::MessageBuffer::MessageHeader::ReceiveListCountType_ToMessageBuffer));
            }

            /* Clear any map alias buffers. */
            for (auto i = 0; i < src_header.GetMapAliasCount(); ++i) {
                offset = dst_msg.Set(offset, ipc::MessageBuffer::MapAliasDescriptor());
            }

            /* Process any raw data. */
            if (const auto raw_count = src_header.GetRawCount(); raw_count != 0) {
                /* Get the offset and size. */
                const size_t offset_words = offset * sizeof(u32);
                const size_t raw_size     = raw_count * sizeof(u32);

                /* Fast case is TLS -> TLS, do raw memcpy if we can. */
                if (!dst_user && !src_user) {
                    std::memcpy(dst_msg_ptr + offset, src_msg_ptr + offset, raw_size);
                } else if (src_user) {
                    /* Determine how much fast size we can copy. */
                    const size_t max_fast_size = std::min<size_t>(offset_words + raw_size, PageSize);
                    const size_t fast_size     = max_fast_size - offset_words;

                    /* Determine the dst permission. User buffer should be unmapped + read, TLS should be user readable. */
                    const KMemoryPermission dst_perm = static_cast<KMemoryPermission>(dst_user ? KMemoryPermission_NotMapped | KMemoryPermission_KernelReadWrite : KMemoryPermission_UserReadWrite);

                    /* Perform the fast part of the copy. */
                    R_TRY(dst_page_table.CopyMemoryFromKernelToLinear(dst_message_buffer + offset_words, fast_size,
                                                                      KMemoryState_FlagReferenceCounted, KMemoryState_FlagReferenceCounted,
                                                                      dst_perm,
                                                                      KMemoryAttribute_Uncached, KMemoryAttribute_None,
                                                                      reinterpret_cast<uintptr_t>(src_msg_ptr) + offset_words));

                    /* If the fast part of the copy didn't get everything, perform the slow part of the copy. */
                    if (fast_size < raw_size) {
                        R_TRY(src_page_table.CopyMemoryFromHeapToHeap(dst_page_table, dst_message_buffer + max_fast_size, raw_size - fast_size,
                                                                      KMemoryState_FlagReferenceCounted, KMemoryState_FlagReferenceCounted,
                                                                      dst_perm,
                                                                      KMemoryAttribute_Uncached, KMemoryAttribute_None,
                                                                      src_message_buffer + max_fast_size,
                                                                      KMemoryState_FlagReferenceCounted, KMemoryState_FlagReferenceCounted,
                                                                      static_cast<KMemoryPermission>(KMemoryPermission_NotMapped | KMemoryPermission_KernelRead),
                                                                      KMemoryAttribute_Uncached | KMemoryAttribute_Locked, KMemoryAttribute_Locked));
                    }
                } else /* if (dst_user) */ {
                    /* The destination is a user buffer, so it should be unmapped + readable. */
                    constexpr KMemoryPermission DestinationPermission = static_cast<KMemoryPermission>(KMemoryPermission_NotMapped | KMemoryPermission_KernelReadWrite);

                    /* Copy the memory. */
                    R_TRY(src_page_table.CopyMemoryFromUserToLinear(dst_message_buffer + offset_words, raw_size,
                                                                    KMemoryState_FlagReferenceCounted, KMemoryState_FlagReferenceCounted,
                                                                    DestinationPermission,
                                                                    KMemoryAttribute_Uncached, KMemoryAttribute_None,
                                                                    src_message_buffer + offset_words));
                }
            }

            /* We succeeded. Perform cleanup with validation. */
            cleanup_guard.Cancel();

            return CleanupMap(request, std::addressof(src_process), std::addressof(dst_page_table));
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

        m_parent->OnServerClosed();

        this->CleanupRequests();

        m_parent->Close();
    }

    Result KServerSession::ReceiveRequest(uintptr_t server_message, uintptr_t server_buffer_size, KPhysicalAddress server_message_paddr) {
        MESOSPHERE_ASSERT_THIS();

        /* Lock the session. */
        KScopedLightLock lk(m_lock);

        /* Get the request and client thread. */
        KSessionRequest *request;
        KScopedAutoObject<KThread> client_thread;
        {
            KScopedSchedulerLock sl;

            /* Ensure that we can service the request. */
            R_UNLESS(!m_parent->IsClientClosed(), svc::ResultSessionClosed());

            /* Ensure we aren't already servicing a request. */
            R_UNLESS(m_current_request == nullptr, svc::ResultNotFound());

            /* Ensure we have a request to service. */
            R_UNLESS(!m_request_list.empty(), svc::ResultNotFound());

            /* Pop the first request from the list. */
            request = std::addressof(m_request_list.front());
            m_request_list.pop_front();

            /* Get the thread for the request. */
            client_thread = KScopedAutoObject<KThread>(request->GetThread());
            R_UNLESS(client_thread.IsNotNull(), svc::ResultSessionClosed());
        }

        /* Set the request as our current. */
        m_current_request = request;

        /* Get the client address. */
        uintptr_t client_message  = request->GetAddress();
        size_t client_buffer_size = request->GetSize();
        bool recv_list_broken = false;

        /* Receive the message. */
        Result result = ReceiveMessage(recv_list_broken, server_message, server_buffer_size, server_message_paddr, *client_thread.GetPointerUnsafe(), client_message, client_buffer_size, this, request);

        /* Handle cleanup on receive failure. */
        if (R_FAILED(result)) {
            /* Cache the result to return it to the client. */
            const Result result_for_client = result;

            /* Clear the current request. */
            {
                KScopedSchedulerLock sl;
                MESOSPHERE_ASSERT(m_current_request == request);
                m_current_request = nullptr;
                if (!m_request_list.empty()) {
                    this->NotifyAvailable();
                }
            }

            /* Reply to the client. */
            {
                /* After we reply, close our reference to the request. */
                ON_SCOPE_EXIT { request->Close(); };

                /* Get the event to check whether the request is async. */
                if (KWritableEvent *event = request->GetEvent(); event != nullptr) {
                    /* The client sent an async request. */
                    KProcess *client = client_thread->GetOwnerProcess();
                    auto &client_pt  = client->GetPageTable();

                    /* Send the async result. */
                    if (R_FAILED(result_for_client)) {
                        ReplyAsyncError(client, client_message, client_buffer_size, result_for_client);
                    }

                    /* Unlock the client buffer. */
                    /* NOTE: Nintendo does not check the result of this. */
                    client_pt.UnlockForIpcUserBuffer(client_message, client_buffer_size);

                    /* Signal the event. */
                    event->Signal();
                } else {
                    /* Set the thread as runnable. */
                    KScopedSchedulerLock sl;
                    if (client_thread->GetState() == KThread::ThreadState_Waiting) {
                        client_thread->SetSyncedObject(nullptr, result_for_client);
                        client_thread->SetState(KThread::ThreadState_Runnable);
                    }
                }
            }

            /* Set the server result. */
            if (recv_list_broken) {
                result = svc::ResultReceiveListBroken();
            } else {
                result = svc::ResultNotFound();
            }
        }

        return result;
    }

    Result KServerSession::SendReply(uintptr_t server_message, uintptr_t server_buffer_size, KPhysicalAddress server_message_paddr) {
        MESOSPHERE_ASSERT_THIS();

        /* Lock the session. */
        KScopedLightLock lk(m_lock);

        /* Get the request. */
        KSessionRequest *request;
        {
            KScopedSchedulerLock sl;

            /* Get the current request. */
            request = m_current_request;
            R_UNLESS(request != nullptr, svc::ResultInvalidState());

            /* Clear the current request, since we're processing it. */
            m_current_request = nullptr;
            if (!m_request_list.empty()) {
                this->NotifyAvailable();
            }
        }

        /* Close reference to the request once we're done processing it. */
        ON_SCOPE_EXIT { request->Close(); };

        /* Extract relevant information from the request. */
        const uintptr_t client_message  = request->GetAddress();
        const size_t client_buffer_size = request->GetSize();
        KThread *client_thread          = request->GetThread();
        KWritableEvent *event           = request->GetEvent();

        /* Check whether we're closed. */
        const bool closed = (client_thread == nullptr || m_parent->IsClientClosed());

        Result result;
        if (!closed) {
            /* If we're not closed, send the reply. */
            result = SendMessage(server_message, server_buffer_size, server_message_paddr, *client_thread, client_message, client_buffer_size, this, request);
        } else {
            /* Otherwise, we'll need to do some cleanup. */
            KProcess *server_process             = request->GetServerProcess();
            KProcess *client_process             = (client_thread != nullptr) ? client_thread->GetOwnerProcess() : nullptr;
            KProcessPageTable *client_page_table = (client_process != nullptr) ? std::addressof(client_process->GetPageTable()) : nullptr;

            /* Cleanup server handles. */
            result = CleanupServerHandles(server_message, server_buffer_size, server_message_paddr);

            /* Cleanup mappings. */
            Result cleanup_map_result = CleanupMap(request, server_process, client_page_table);

            /* If we successfully cleaned up handles, use the map cleanup result as our result. */
            if (R_SUCCEEDED(result)) {
                result = cleanup_map_result;
            }
        }

        /* Select a result for the client. */
        Result client_result = result;
        if (closed && R_SUCCEEDED(result)) {
            result        = svc::ResultSessionClosed();
            client_result = svc::ResultSessionClosed();
        } else {
            result = ResultSuccess();
        }

        /* If there's a client thread, update it. */
        if (client_thread != nullptr) {
            if (event != nullptr) {
                /* Get the client process/page table. */
                KProcess *client_process             = client_thread->GetOwnerProcess();
                KProcessPageTable *client_page_table = std::addressof(client_process->GetPageTable());

                /* If we need to, reply with an async error. */
                if (R_FAILED(client_result)) {
                    ReplyAsyncError(client_process, client_message, client_buffer_size, client_result);
                }

                /* Unlock the client buffer. */
                /* NOTE: Nintendo does not check the result of this. */
                client_page_table->UnlockForIpcUserBuffer(client_message, client_buffer_size);

                /* Signal the event. */
                event->Signal();
            } else {
                /* Set the thread as runnable. */
                KScopedSchedulerLock sl;
                if (client_thread->GetState() == KThread::ThreadState_Waiting) {
                    client_thread->SetSyncedObject(nullptr, client_result);
                    client_thread->SetState(KThread::ThreadState_Runnable);
                }
            }
        }

        return result;
    }

    Result KServerSession::OnRequest(KSessionRequest *request) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Ensure that we can handle new requests. */
        R_UNLESS(!m_parent->IsServerClosed(), svc::ResultSessionClosed());

        /* If there's no event, this is synchronous, so we should check for thread termination. */
        if (request->GetEvent() == nullptr) {
            KThread *thread = request->GetThread();
            R_UNLESS(!thread->IsTerminationRequested(), svc::ResultTerminationRequested());
            thread->SetState(KThread::ThreadState_Waiting);
        }

        /* Get whether we're empty. */
        const bool was_empty = m_request_list.empty();

        /* Add the request to the list. */
        request->Open();
        m_request_list.push_back(*request);

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
        if (m_parent->IsClientClosed()) {
            return true;
        }

        /* Otherwise, we're signaled if we have a request and aren't handling one. */
        return !m_request_list.empty() && m_current_request == nullptr;
    }

    bool KServerSession::IsSignaled() const {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        return this->IsSignaledImpl();
    }

    void KServerSession::CleanupRequests() {
        MESOSPHERE_ASSERT_THIS();

        KScopedLightLock lk(m_lock);

        /* Clean up any pending requests. */
        while (true) {
            /* Get the next request. */
            KSessionRequest *request = nullptr;
            {
                KScopedSchedulerLock sl;

                if (m_current_request) {
                    /* Choose the current request if we have one. */
                    request = m_current_request;
                    m_current_request = nullptr;
                } else if (!m_request_list.empty()) {
                    /* Pop the request from the front of the list. */
                    request = std::addressof(m_request_list.front());
                    m_request_list.pop_front();
                }
            }

            /* If there's no request, we're done. */
            if (request == nullptr) {
                break;
            }

            /* Close a reference to the request once it's cleaned up. */
            ON_SCOPE_EXIT { request->Close(); };

            /* Extract relevant information from the request. */
            const uintptr_t client_message  = request->GetAddress();
            const size_t client_buffer_size = request->GetSize();
            KThread *client_thread          = request->GetThread();
            KWritableEvent *event           = request->GetEvent();

            KProcess *server_process             = request->GetServerProcess();
            KProcess *client_process             = (client_thread != nullptr) ? client_thread->GetOwnerProcess() : nullptr;
            KProcessPageTable *client_page_table = (client_process != nullptr) ? std::addressof(client_process->GetPageTable()) : nullptr;

            /* Cleanup the mappings. */
            Result result = CleanupMap(request, server_process, client_page_table);

            /* If there's a client thread, update it. */
            if (client_thread != nullptr) {
                if (event != nullptr) {
                    /* We need to reply async. */
                    ReplyAsyncError(client_process, client_message, client_buffer_size, (R_SUCCEEDED(result) ? svc::ResultSessionClosed() : result));

                    /* Unlock the client buffer. */
                    /* NOTE: Nintendo does not check the result of this. */
                    client_page_table->UnlockForIpcUserBuffer(client_message, client_buffer_size);

                    /* Signal the event. */
                    event->Signal();
                } else {
                    /* Set the thread as runnable. */
                    KScopedSchedulerLock sl;
                    if (client_thread->GetState() == KThread::ThreadState_Waiting) {
                        client_thread->SetSyncedObject(nullptr, (R_SUCCEEDED(result) ? svc::ResultSessionClosed() : result));
                        client_thread->SetState(KThread::ThreadState_Runnable);
                    }
                }
            }
        }
    }

    void KServerSession::OnClientClosed() {
        MESOSPHERE_ASSERT_THIS();

        KScopedLightLock lk(m_lock);

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

                if (m_current_request != nullptr && m_current_request != prev_request) {
                    /* Set the request, open a reference as we process it. */
                    request = m_current_request;
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
                } else if (!m_request_list.empty()) {
                    /* Pop the request from the front of the list. */
                    request = std::addressof(m_request_list.front());
                    m_request_list.pop_front();

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
                KProcess *client_process = thread->GetOwnerProcess();
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
        this->NotifyAvailable(svc::ResultSessionClosed());
    }

    #pragma GCC pop_options

    void KServerSession::Dump() {
        MESOSPHERE_ASSERT_THIS();

        KScopedLightLock lk(m_lock);
        {
            KScopedSchedulerLock sl;
            MESOSPHERE_RELEASE_LOG("Dump Session %p\n", this);

            /* Dump current request. */
            bool has_request = false;
            if (m_current_request != nullptr) {
                KThread *thread = m_current_request->GetThread();
                const s32 thread_id = thread != nullptr ? static_cast<s32>(thread->GetId()) : -1;
                MESOSPHERE_RELEASE_LOG("    CurrentReq %p Thread=%p ID=%d\n", m_current_request, thread, thread_id);
                has_request = true;
            }

            /* Dump all rqeuests in list. */
            for (auto it = m_request_list.begin(); it != m_request_list.end(); ++it) {
                KThread *thread = it->GetThread();
                const s32 thread_id = thread != nullptr ? static_cast<s32>(thread->GetId()) : -1;
                MESOSPHERE_RELEASE_LOG("    Req %p Thread=%p ID=%d\n", m_current_request, thread, thread_id);
                has_request = true;
            }

            /* If we didn't have any requests, print so. */
            if (!has_request) {
                MESOSPHERE_RELEASE_LOG("    None\n");
            }
        }
    }

}
