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

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {

        ALWAYS_INLINE Result ReplyAndReceiveImpl(int32_t *out_index, uintptr_t message, size_t buffer_size, KPhysicalAddress message_paddr, KSynchronizationObject **objs, int32_t num_objects, ams::svc::Handle reply_target, int64_t timeout_ns) {
            /* Reply to the target, if one is specified. */
            if (reply_target != ams::svc::InvalidHandle) {
                /* TODO */
                MESOSPHERE_UNIMPLEMENTED();
            }

            /* Receive a message. */
            {
                /* Convert the timeout from nanoseconds to ticks. */
                /* NOTE: Nintendo does not use this conversion logic in WaitSynchronization... */
                s64 timeout;
                if (timeout_ns > 0) {
                    const ams::svc::Tick offset_tick(TimeSpan::FromNanoSeconds(timeout_ns));
                    if (AMS_LIKELY(offset_tick > 0)) {
                        timeout = KHardwareTimer::GetTick() + offset_tick + 2;
                        if (AMS_UNLIKELY(timeout <= 0)) {
                            timeout = std::numeric_limits<s64>::max();
                        }
                    } else {
                        timeout = std::numeric_limits<s64>::max();
                    }
                } else {
                    timeout = timeout_ns;
                }

                /* Wait for a message. */
                while (true) {
                    s32 index;
                    Result result = Kernel::GetSynchronization().Wait(std::addressof(index), objs, num_objects, timeout);
                    if (svc::ResultTimedOut::Includes(result)) {
                        return result;
                    }

                    if (R_SUCCEEDED(result)) {
                        KServerSession *session = objs[index]->DynamicCast<KServerSession *>();
                        if (session != nullptr) {
                            result = session->ReceiveRequest(message, buffer_size, message_paddr);
                            if (svc::ResultNotFound::Includes(result)) {
                                continue;
                            }
                        }
                    }

                    *out_index = index;
                    return result;
                }
            }
        }

        ALWAYS_INLINE Result ReplyAndReceiveImpl(int32_t *out_index, uintptr_t message, size_t buffer_size, KPhysicalAddress message_paddr, KUserPointer<const ams::svc::Handle *> user_handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
            /* Ensure number of handles is valid. */
            R_UNLESS(0 <= num_handles && num_handles <= ams::svc::ArgumentHandleCountMax, svc::ResultOutOfRange());

            /* Get the synchronization context. */
            auto &handle_table = GetCurrentProcess().GetHandleTable();
            KSynchronizationObject **objs = GetCurrentThread().GetSynchronizationObjectBuffer();
            ams::svc::Handle *handles = GetCurrentThread().GetHandleBuffer();

            /* Copy user handles. */
            if (num_handles > 0) {
                /* Ensure that we can try to get the handles. */
                R_UNLESS(GetCurrentProcess().GetPageTable().Contains(KProcessAddress(user_handles.GetUnsafePointer()), num_handles * sizeof(ams::svc::Handle)), svc::ResultInvalidPointer());

                /* Get the handles. */
                R_TRY(user_handles.CopyArrayTo(handles, num_handles));

                /* Convert the handles to objects. */
                R_UNLESS(handle_table.GetMultipleObjects<KSynchronizationObject>(objs, handles, num_handles), svc::ResultInvalidHandle());
            }

            /* Ensure handles are closed when we're done. */
            ON_SCOPE_EXIT {
                for (auto i = 0; i < num_handles; ++i) {
                    objs[i]->Close();
                }
            };

            return ReplyAndReceiveImpl(out_index, message, buffer_size, message_paddr, objs, num_handles, reply_target, timeout_ns);
        }

        ALWAYS_INLINE Result ReplyAndReceive(int32_t *out_index, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
            return ReplyAndReceiveImpl(out_index, 0, 0, Null<KPhysicalAddress>, handles, num_handles, reply_target, timeout_ns);
        }

    }

    /* =============================    64 ABI    ============================= */

    Result SendSyncRequest64(ams::svc::Handle session_handle) {
        MESOSPHERE_PANIC("Stubbed SvcSendSyncRequest64 was called.");
    }

    Result SendSyncRequestWithUserBuffer64(ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, ams::svc::Handle session_handle) {
        MESOSPHERE_PANIC("Stubbed SvcSendSyncRequestWithUserBuffer64 was called.");
    }

    Result SendAsyncRequestWithUserBuffer64(ams::svc::Handle *out_event_handle, ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, ams::svc::Handle session_handle) {
        MESOSPHERE_PANIC("Stubbed SvcSendAsyncRequestWithUserBuffer64 was called.");
    }

    Result ReplyAndReceive64(int32_t *out_index, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
        return ReplyAndReceive(out_index, handles, num_handles, reply_target, timeout_ns);
    }

    Result ReplyAndReceiveWithUserBuffer64(int32_t *out_index, ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
        MESOSPHERE_PANIC("Stubbed SvcReplyAndReceiveWithUserBuffer64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    Result SendSyncRequest64From32(ams::svc::Handle session_handle) {
        MESOSPHERE_PANIC("Stubbed SvcSendSyncRequest64From32 was called.");
    }

    Result SendSyncRequestWithUserBuffer64From32(ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, ams::svc::Handle session_handle) {
        MESOSPHERE_PANIC("Stubbed SvcSendSyncRequestWithUserBuffer64From32 was called.");
    }

    Result SendAsyncRequestWithUserBuffer64From32(ams::svc::Handle *out_event_handle, ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, ams::svc::Handle session_handle) {
        MESOSPHERE_PANIC("Stubbed SvcSendAsyncRequestWithUserBuffer64From32 was called.");
    }

    Result ReplyAndReceive64From32(int32_t *out_index, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
        return ReplyAndReceive(out_index, handles, num_handles, reply_target, timeout_ns);
    }

    Result ReplyAndReceiveWithUserBuffer64From32(int32_t *out_index, ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
        MESOSPHERE_PANIC("Stubbed SvcReplyAndReceiveWithUserBuffer64From32 was called.");
    }

}
