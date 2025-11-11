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

#pragma GCC push_options
#pragma GCC optimize ("-O3")

namespace ams::kern::svc {

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
            R_RETURN(session->SendSyncRequest(message, buffer_size));
        }

        ALWAYS_INLINE Result ReplyAndReceiveImpl(int32_t *out_index, uintptr_t message, size_t buffer_size, KPhysicalAddress message_paddr, KSynchronizationObject **objs, int32_t num_objects, ams::svc::Handle reply_target, int64_t timeout_ns) {
            /* Reply to the target, if one is specified. */
            if (reply_target != ams::svc::InvalidHandle) {
                KScopedAutoObject session = GetCurrentProcess().GetHandleTable().GetObject<KServerSession>(reply_target);
                R_UNLESS(session.IsNotNull(), svc::ResultInvalidHandle());

                /* If we fail to reply, we want to set the output index to -1. */
                ON_RESULT_FAILURE { *out_index = -1; };

                /* Send the reply. */
                R_TRY(session->SendReply(message, buffer_size, message_paddr));
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
                    /* Close any pending objects before we wait. */
                    GetCurrentThread().DestroyClosedObjects();

                    /* Wait for an object. */
                    s32 index;
                    Result result = KSynchronizationObject::Wait(std::addressof(index), objs, num_objects, timeout);
                    if (svc::ResultTimedOut::Includes(result)) {
                        R_THROW(result);
                    }

                    /* Receive the request. */
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
                    R_RETURN(result);
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

            R_RETURN(ReplyAndReceiveImpl(out_index, message, buffer_size, message_paddr, objs, num_handles, reply_target, timeout_ns));
        }

        ALWAYS_INLINE Result SendSyncRequest(ams::svc::Handle session_handle) {
            R_RETURN(SendSyncRequestImpl(0, 0, session_handle));
        }

        ALWAYS_INLINE Result SendSyncRequestWithUserBuffer(uintptr_t message, size_t buffer_size, ams::svc::Handle session_handle) {
            /* Validate that the message buffer is page aligned and does not overflow. */
            R_UNLESS(util::IsAligned(message, PageSize),     svc::ResultInvalidAddress());
            R_UNLESS(buffer_size > 0,                        svc::ResultInvalidSize());
            R_UNLESS(util::IsAligned(buffer_size, PageSize), svc::ResultInvalidSize());
            R_UNLESS(message < message + buffer_size,        svc::ResultInvalidCurrentMemory());

            /* Get the process page table. */
            auto &page_table = GetCurrentProcess().GetPageTable();

            /* Lock the message buffer. */
            R_TRY(page_table.LockForIpcUserBuffer(nullptr, message, buffer_size));

            {
                /* If we fail to send the message, unlock the message buffer. */
                ON_RESULT_FAILURE { static_cast<void>(page_table.UnlockForIpcUserBuffer(message, buffer_size)); };

                /* Send the request. */
                MESOSPHERE_ASSERT(message != 0);
                R_TRY(SendSyncRequestImpl(message, buffer_size, session_handle));
            }

            /* We successfully processed, so try to unlock the message buffer. */
            R_RETURN(page_table.UnlockForIpcUserBuffer(message, buffer_size));
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

            /* At end of scope, kill the standing event references. */
            ON_SCOPE_EXIT {
                event->GetReadableEvent().Close();
                event->Close();
            };

            /* Register the event. */
            KEvent::Register(event);

            /* Add the readable event to the handle table. */
            R_TRY(handle_table.Add(out_event_handle, std::addressof(event->GetReadableEvent())));

            /* Ensure that if we fail to send the request, we close the readable handle. */
            ON_RESULT_FAILURE { handle_table.Remove(*out_event_handle); };

            /* Send the async request. */
            R_RETURN(session->SendAsyncRequest(event, message, buffer_size));
        }

        ALWAYS_INLINE Result SendAsyncRequestWithUserBuffer(ams::svc::Handle *out_event_handle, uintptr_t message, size_t buffer_size, ams::svc::Handle session_handle) {
            /* Validate that the message buffer is page aligned and does not overflow. */
            R_UNLESS(util::IsAligned(message, PageSize),     svc::ResultInvalidAddress());
            R_UNLESS(buffer_size > 0,                        svc::ResultInvalidSize());
            R_UNLESS(util::IsAligned(buffer_size, PageSize), svc::ResultInvalidSize());
            R_UNLESS(message < message + buffer_size,        svc::ResultInvalidCurrentMemory());

            /* Get the process page table. */
            auto &page_table = GetCurrentProcess().GetPageTable();

            /* Lock the message buffer. */
            R_TRY(page_table.LockForIpcUserBuffer(nullptr, message, buffer_size));

            /* Ensure that if we fail and aren't terminating that we unlock the user buffer. */
            ON_RESULT_FAILURE_BESIDES(svc::ResultTerminationRequested) {
                static_cast<void>(page_table.UnlockForIpcUserBuffer(message, buffer_size));
            };

            /* Send the request. */
            MESOSPHERE_ASSERT(message != 0);
            R_RETURN(SendAsyncRequestWithUserBufferImpl(out_event_handle, message, buffer_size, session_handle));
        }

        ALWAYS_INLINE Result ReplyAndReceive(int32_t *out_index, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
            R_RETURN(ReplyAndReceiveImpl(out_index, 0, 0, Null<KPhysicalAddress>, handles, num_handles, reply_target, timeout_ns));
        }

        ALWAYS_INLINE Result ReplyAndReceiveWithUserBuffer(int32_t *out_index, uintptr_t message, size_t buffer_size, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
            /* Validate that the message buffer is page aligned and does not overflow. */
            R_UNLESS(util::IsAligned(message, PageSize),     svc::ResultInvalidAddress());
            R_UNLESS(buffer_size > 0,                        svc::ResultInvalidSize());
            R_UNLESS(util::IsAligned(buffer_size, PageSize), svc::ResultInvalidSize());
            R_UNLESS(message < message + buffer_size,        svc::ResultInvalidCurrentMemory());

            /* Get the process page table. */
            auto &page_table = GetCurrentProcess().GetPageTable();

            /* Lock the message buffer, getting its physical address. */
            KPhysicalAddress message_paddr;
            R_TRY(page_table.LockForIpcUserBuffer(std::addressof(message_paddr), message, buffer_size));

            {
                /* If we fail to send the message, unlock the message buffer. */
                ON_RESULT_FAILURE { static_cast<void>(page_table.UnlockForIpcUserBuffer(message, buffer_size)); };

                /* Reply/Receive the request. */
                MESOSPHERE_ASSERT(message != 0);
                R_TRY(ReplyAndReceiveImpl(out_index, message, buffer_size, message_paddr, handles, num_handles, reply_target, timeout_ns));
            }

            /* We successfully processed, so try to unlock the message buffer. */
            R_RETURN(page_table.UnlockForIpcUserBuffer(message, buffer_size));
        }

    }

    /* =============================    64 ABI    ============================= */

    Result SendSyncRequest64(ams::svc::Handle session_handle) {
        R_RETURN(SendSyncRequest(session_handle));
    }

    Result SendSyncRequestWithUserBuffer64(ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, ams::svc::Handle session_handle) {
        R_RETURN(SendSyncRequestWithUserBuffer(message_buffer, message_buffer_size, session_handle));
    }

    Result SendAsyncRequestWithUserBuffer64(ams::svc::Handle *out_event_handle, ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, ams::svc::Handle session_handle) {
        R_RETURN(SendAsyncRequestWithUserBuffer(out_event_handle, message_buffer, message_buffer_size, session_handle));
    }

    Result ReplyAndReceive64(int32_t *out_index, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
        R_RETURN(ReplyAndReceive(out_index, handles, num_handles, reply_target, timeout_ns));
    }

    Result ReplyAndReceiveWithUserBuffer64(int32_t *out_index, ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
        R_RETURN(ReplyAndReceiveWithUserBuffer(out_index, message_buffer, message_buffer_size, handles, num_handles, reply_target, timeout_ns));
    }

    /* ============================= 64From32 ABI ============================= */

    Result SendSyncRequest64From32(ams::svc::Handle session_handle) {
        R_RETURN(SendSyncRequest(session_handle));
    }

    Result SendSyncRequestWithUserBuffer64From32(ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, ams::svc::Handle session_handle) {
        R_RETURN(SendSyncRequestWithUserBuffer(message_buffer, message_buffer_size, session_handle));
    }

    Result SendAsyncRequestWithUserBuffer64From32(ams::svc::Handle *out_event_handle, ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, ams::svc::Handle session_handle) {
        R_RETURN(SendAsyncRequestWithUserBuffer(out_event_handle, message_buffer, message_buffer_size, session_handle));
    }

    Result ReplyAndReceive64From32(int32_t *out_index, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
        R_RETURN(ReplyAndReceive(out_index, handles, num_handles, reply_target, timeout_ns));
    }

    Result ReplyAndReceiveWithUserBuffer64From32(int32_t *out_index, ams::svc::Address message_buffer, ams::svc::Size message_buffer_size, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, ams::svc::Handle reply_target, int64_t timeout_ns) {
        R_RETURN(ReplyAndReceiveWithUserBuffer(out_index, message_buffer, message_buffer_size, handles, num_handles, reply_target, timeout_ns));
    }

}

#pragma GCC pop_options
