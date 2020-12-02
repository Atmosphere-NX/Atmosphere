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

    #pragma GCC push_options
    #pragma GCC optimize ("-O3")

    /* =============================    Common    ============================= */

    namespace {

        ALWAYS_INLINE Result SendSyncRequestImpl(uintptr_t message, size_t buffer_size, ams::svc::Handle session_handle) {
            /* Get the client session. */
            KScopedAutoObject session = GetCurrentProcess().GetHandleTable().GetObject<KClientSession>(session_handle);
            R_UNLESS(session.IsNotNull(), svc::ResultInvalidHandle());

            /* Get the parent, and persist a reference to it until we're done. */
            KScopedAutoObject parent = session->GetParent();
            MESOSPHERE_ASSERT(parent.IsNotNull());

            /* Send the request. */
            return session->SendSyncRequest(message, buffer_size);
        }

        ALWAYS_INLINE Result ReplyAndReceiveImpl(int32_t *out_index, uintptr_t message, size_t buffer_size, KPhysicalAddress message_paddr, KSynchronizationObject **objs, int32_t num_objects, ams::svc::Handle reply_target, int64_t timeout_ns) {
            /* Reply to the target, if one is specified. */
            if (reply_target != ams::svc::InvalidHandle) {
                KScopedAutoObject session = GetCurrentProcess().GetHandleTable().GetObject<KServerSession>(reply_target);
                R_UNLESS(session.IsNotNull(), svc::ResultInvalidHandle());

                /* If we fail to reply, we want to set the output index to -1. */
                auto reply_idx_guard = SCOPE_GUARD { *out_index = -1; };

                /* Send the reply. */
                R_TRY(session->SendReply(message, buffer_size, message_paddr));

                /* Cancel our guard. */
                reply_idx_guard.Cancel();
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
                    Result result = KSynchronizationObject::Wait(std::addressof(index), objs, num_objects, timeout);
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

        ALWAYS_INLINE Result SendSyncRequest(ams::svc::Handle session_handle) {
            return SendSyncRequestImpl(0, 0, session_handle);
        }

        ALWAYS_INLINE Result SendSyncRequestWithUserBuffer(uintptr_t message, size_t buffer_size, ams::svc::Handle session_handle) {
            /* Validate that the message buffer is page aligned and does not overflow. */
            R_UNLESS(util::IsAligned(message, PageSize),     svc::ResultInvalidAddress());
            R_UNLESS(buffer_size > 0,                        svc::ResultInvalidSize());
            R_UNLESS(util::IsAligned(buffer_size, PageSize), svc::ResultInvalidSize());
            R_UNLESS(message < message + buffer_size,        svc::ResultInvalidCurrentMemory());

            /* Get the process page table. */
            auto &page_table = GetCurrentProcess().GetPageTable();

            /* Lock the mesage buffer. */
            R_TRY(page_table.LockForIpcUserBuffer(nullptr, message, buffer_size));

            /* Ensure that even if we fail, we unlock the message buffer when done. */
            auto unlock_guard = SCOPE_GUARD { page_table.UnlockForIpcUserBuffer(message, buffer_size); };

            /* Send the request. */
            MESOSPHERE_ASSERT(message != 0);
            R_TRY(SendSyncRequestImpl(message, buffer_size, session_handle));

            /* We sent the request successfully, so cancel our guard and check the unlock result. */
            unlock_guard.Cancel();
            return page_table.UnlockForIpcUserBuffer(message, buffer_size);
        }

        ALWAYS_INLINE Result SendAsyncRequestWithUserBufferImpl(ams::svc::Handle *out_event_handle, uintptr_t message, size_t buffer_size, ams::svc::Handle session_handle) {
            /* Get the process and handle table. */
            auto &process      = GetCurrentProcess();
            auto &handle_table = process.GetHandleTable();

            /* Reserve a new event from the process resource limit. */
            KScopedResourceReservation event_reservation(std::addressof(process), ams::svc::LimitableResource_EventCountMax);
            R_UNLESS(event_reservation.Succeeded(), svc::ResultLimitReached());

            /* Get the client session. */
            KScopedAutoObject session = GetCurrentProcess().GetHandleTable().GetObject<KClientSession>(session_handle);
            R_UNLESS(session.IsNotNull(), svc::ResultInvalidHandle());

            /* Get the parent, and persist a reference to it until we're done. */
            KScopedAutoObject parent = session->GetParent();
            MESOSPHERE_ASSERT(parent.IsNotNull());

            /* Create a new event. */
            KEvent *event = KEvent::Create();
            R_UNLESS(event != nullptr, svc::ResultOutOfResource());

            /* Initialize the event. */
            event->Initialize();

            /* Commit our reservation. */
            event_reservation.Commit();

            /* At end of scope, kill the standing references to the sub events. */
            ON_SCOPE_EXIT {
                event->GetReadableEvent().Close();
                event->GetWritableEvent().Close();
            };

            /* Register the event. */
            KEvent::Register(event);

            /* Add the readable event to the handle table. */
            R_TRY(handle_table.Add(out_event_handle, std::addressof(event->GetReadableEvent())));

            /* Ensure that if we fail to send the request, we close the readable handle. */
            auto read_guard = SCOPE_GUARD { handle_table.Remove(*out_event_handle); };

            /* Send the async request. */
            R_TRY(session->SendAsyncRequest(std::addressof(event->GetWritableEvent()), message, buffer_size));

            /* We succeeded. */
            read_guard.Cancel();
            return ResultSuccess();
        }

        ALWAYS_INLINE Result SendAsyncRequestWithUserBuffer(ams::svc::Handle *out_event_handle, uintptr_t message, size_t buffer_size, ams::svc::Handle session_handle) {
            /* Validate that the message buffer is page aligned and does not overflow. */
            R_UNLESS(util::IsAligned(message, PageSize),     svc::ResultInvalidAddress());
            R_UNLESS(buffer_size > 0,                        svc::ResultInvalidSize());
            R_UNLESS(util::IsAligned(buffer_size, PageSize), svc::ResultInvalidSize());
            R_UNLESS(message < message + buffer_size,        svc::ResultInvalidCurrentMemory());

            /* Get the process page table. */
            auto &page_table = GetCurrentProcess().GetPageTable();

            /* Lock the mesage buffer. */
            R_TRY(page_table.LockForIpcUserBuffer(nullptr, message, buffer_size));

            /* Ensure that if we fail, we unlock the message buffer. */
            auto unlock_guard = SCOPE_GUARD { page_table.UnlockForIpcUserBuffer(message, buffer_size); };

            /* Send the request. */
            MESOSPHERE_ASSERT(message != 0);
            R_TRY(SendAsyncRequestWithUserBufferImpl(out_event_handle, message, buffer_size, session_handle));

            /* We sent the request successfully. */
            unlock_guard.Cancel();
            return ResultSuccess();
        }

        ALWAYS_INLINE Result ReplyAndReceive(int32_t *out_index, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
            return ReplyAndReceiveImpl(out_index, 0, 0, Null<KPhysicalAddress>, handles, num_handles, reply_target, timeout_ns);
        }

        ALWAYS_INLINE Result ReplyAndReceiveWithUserBuffer(int32_t *out_index, uintptr_t message, size_t buffer_size, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
            /* Validate that the message buffer is page aligned and does not overflow. */
            R_UNLESS(util::IsAligned(message, PageSize),     svc::ResultInvalidAddress());
            R_UNLESS(buffer_size > 0,                        svc::ResultInvalidSize());
            R_UNLESS(util::IsAligned(buffer_size, PageSize), svc::ResultInvalidSize());
            R_UNLESS(message < message + buffer_size,        svc::ResultInvalidCurrentMemory());

            /* Get the process page table. */
            auto &page_table = GetCurrentProcess().GetPageTable();

            /* Lock the mesage buffer, getting its physical address. */
            KPhysicalAddress message_paddr;
            R_TRY(page_table.LockForIpcUserBuffer(std::addressof(message_paddr), message, buffer_size));

            /* Ensure that even if we fail, we unlock the message buffer when done. */
            auto unlock_guard = SCOPE_GUARD { page_table.UnlockForIpcUserBuffer(message, buffer_size); };

            /* Send the request. */
            MESOSPHERE_ASSERT(message != 0);
            R_TRY(ReplyAndReceiveImpl(out_index, message, buffer_size, message_paddr, handles, num_handles, reply_target, timeout_ns));

            /* We sent the request successfully, so cancel our guard and check the unlock result. */
            unlock_guard.Cancel();
            return page_table.UnlockForIpcUserBuffer(message, buffer_size);
        }

    }

    /* =============================    64 ABI    ============================= */

    Result SendSyncRequest64(ams::svc::Handle session_handle) {
        return SendSyncRequest(session_handle);
    }

    Result SendSyncRequestWithUserBuffer64(ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, ams::svc::Handle session_handle) {
        return SendSyncRequestWithUserBuffer(message_buffer, message_buffer_size, session_handle);
    }

    Result SendAsyncRequestWithUserBuffer64(ams::svc::Handle *out_event_handle, ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, ams::svc::Handle session_handle) {
        return SendAsyncRequestWithUserBuffer(out_event_handle, message_buffer, message_buffer_size, session_handle);
    }

    Result ReplyAndReceive64(int32_t *out_index, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
        return ReplyAndReceive(out_index, handles, num_handles, reply_target, timeout_ns);
    }

    Result ReplyAndReceiveWithUserBuffer64(int32_t *out_index, ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
        return ReplyAndReceiveWithUserBuffer(out_index, message_buffer, message_buffer_size, handles, num_handles, reply_target, timeout_ns);
    }

    /* ============================= 64From32 ABI ============================= */

    Result SendSyncRequest64From32(ams::svc::Handle session_handle) {
        return SendSyncRequest(session_handle);
    }

    Result SendSyncRequestWithUserBuffer64From32(ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, ams::svc::Handle session_handle) {
        return SendSyncRequestWithUserBuffer(message_buffer, message_buffer_size, session_handle);
    }

    Result SendAsyncRequestWithUserBuffer64From32(ams::svc::Handle *out_event_handle, ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, ams::svc::Handle session_handle) {
        return SendAsyncRequestWithUserBuffer(out_event_handle, message_buffer, message_buffer_size, session_handle);
    }

    Result ReplyAndReceive64From32(int32_t *out_index, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
        return ReplyAndReceive(out_index, handles, num_handles, reply_target, timeout_ns);
    }

    Result ReplyAndReceiveWithUserBuffer64From32(int32_t *out_index, ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
        return ReplyAndReceiveWithUserBuffer(out_index, message_buffer, message_buffer_size, handles, num_handles, reply_target, timeout_ns);
    }

    #pragma GCC pop_options

}
