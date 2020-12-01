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

        Result CloseHandle(ams::svc::Handle handle) {
            /* Remove the handle. */
            R_UNLESS(GetCurrentProcess().GetHandleTable().Remove(handle), svc::ResultInvalidHandle());
            return ResultSuccess();
        }

        Result ResetSignal(ams::svc::Handle handle) {
            /* Get the current handle table. */
            auto &handle_table = GetCurrentProcess().GetHandleTable();

            /* Try to reset as readable event. */
            {
                KScopedAutoObject readable_event = handle_table.GetObject<KReadableEvent>(handle);
                if (readable_event.IsNotNull()) {
                    return readable_event->Reset();
                }
            }

            /* Try to reset as process. */
            {
                KScopedAutoObject process = handle_table.GetObject<KProcess>(handle);
                if (process.IsNotNull()) {
                    return process->Reset();
                }
            }

            return svc::ResultInvalidHandle();
        }

        Result WaitSynchronizationImpl(int32_t *out_index, KSynchronizationObject **objs, int32_t num_handles, int64_t timeout_ns) {
            /* Convert the timeout from nanoseconds to ticks. */
            s64 timeout;
            if (timeout_ns > 0) {
                u64 ticks = KHardwareTimer::GetTick();
                ticks += ams::svc::Tick(TimeSpan::FromNanoSeconds(timeout_ns));
                ticks += 2;

                timeout = ticks;
            } else {
                timeout = timeout_ns;
            }

            return KSynchronizationObject::Wait(out_index, objs, num_handles, timeout);
        }

        Result WaitSynchronization(int32_t *out_index, KUserPointer<const ams::svc::Handle *> user_handles, int32_t num_handles, int64_t timeout_ns) {
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

            /* Wait on the objects. */
            R_TRY_CATCH(WaitSynchronizationImpl(out_index, objs, num_handles, timeout_ns)) {
                R_CONVERT(svc::ResultSessionClosed, ResultSuccess())
            } R_END_TRY_CATCH;

            return ResultSuccess();
        }

        Result CancelSynchronization(ams::svc::Handle handle) {
            /* Get the thread from its handle. */
            KScopedAutoObject thread = GetCurrentProcess().GetHandleTable().GetObject<KThread>(handle);
            R_UNLESS(thread.IsNotNull(), svc::ResultInvalidHandle());

            /* Cancel the thread's wait. */
            thread->WaitCancel();
            return ResultSuccess();
        }

        void SynchronizePreemptionState() {
            /* Lock the scheduler. */
            KScopedSchedulerLock sl;

            /* If the current thread is pinned, unpin it. */
            KProcess *cur_process = GetCurrentProcessPointer();
            if (cur_process->GetPinnedThread(GetCurrentCoreId()) == GetCurrentThreadPointer()) {
                /* Clear the current thread's interrupt flag. */
                GetCurrentThread().ClearInterruptFlag();

                /* Unpin the current thread. */
                cur_process->UnpinCurrentThread();
            }
        }

    }

    /* =============================    64 ABI    ============================= */

    Result CloseHandle64(ams::svc::Handle handle) {
        return CloseHandle(handle);
    }

    Result ResetSignal64(ams::svc::Handle handle) {
        return ResetSignal(handle);
    }

    Result WaitSynchronization64(int32_t *out_index, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, int64_t timeout_ns) {
        return WaitSynchronization(out_index, handles, num_handles, timeout_ns);
    }

    Result CancelSynchronization64(ams::svc::Handle handle) {
        return CancelSynchronization(handle);
    }

    void SynchronizePreemptionState64() {
        return SynchronizePreemptionState();
    }

    /* ============================= 64From32 ABI ============================= */

    Result CloseHandle64From32(ams::svc::Handle handle) {
        return CloseHandle(handle);
    }

    Result ResetSignal64From32(ams::svc::Handle handle) {
        return ResetSignal(handle);
    }

    Result WaitSynchronization64From32(int32_t *out_index, KUserPointer<const ams::svc::Handle *> handles, int32_t num_handles, int64_t timeout_ns) {
        return WaitSynchronization(out_index, handles, num_handles, timeout_ns);
    }

    Result CancelSynchronization64From32(ams::svc::Handle handle) {
        return CancelSynchronization(handle);
    }

    void SynchronizePreemptionState64From32() {
        return SynchronizePreemptionState();
    }

}
