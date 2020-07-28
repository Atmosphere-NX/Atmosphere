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

        constexpr bool IsValidCoreId(int32_t core_id) {
            return (0 <= core_id && core_id < static_cast<int32_t>(cpu::NumCores));
        }

        Result CreateThread(ams::svc::Handle *out, ams::svc::ThreadFunc f, uintptr_t arg, uintptr_t stack_bottom, int32_t priority, int32_t core_id) {
            /* Adjust core id, if it's the default magic. */
            KProcess &process = GetCurrentProcess();
            if (core_id == ams::svc::IdealCoreUseProcessValue) {
                core_id = process.GetIdealCoreId();
            }

            /* Validate arguments. */
            R_UNLESS(IsValidCoreId(core_id),                          svc::ResultInvalidCoreId());
            R_UNLESS(((1ul << core_id) & process.GetCoreMask()) != 0, svc::ResultInvalidCoreId());

            R_UNLESS(ams::svc::HighestThreadPriority <= priority && priority <= ams::svc::LowestThreadPriority, svc::ResultInvalidPriority());
            R_UNLESS(process.CheckThreadPriority(priority),                                                     svc::ResultInvalidPriority());

            /* Reserve a new thread from the process resource limit (waiting up to 100ms). */
            KScopedResourceReservation thread_reservation(std::addressof(process), ams::svc::LimitableResource_ThreadCountMax, 1, KHardwareTimer::GetTick() + ams::svc::Tick(TimeSpan::FromMilliSeconds(100)));
            R_UNLESS(thread_reservation.Succeeded(), svc::ResultLimitReached());

            /* Create the thread. */
            KScopedAutoObject thread = KThread::Create();
            R_UNLESS(thread.IsNotNull(), svc::ResultOutOfResource());

            /* Initialize the thread. */
            {
                KScopedLightLock lk(process.GetStateLock());
                R_TRY(KThread::InitializeUserThread(thread.GetPointerUnsafe(), reinterpret_cast<KThreadFunction>(static_cast<uintptr_t>(f)), arg, stack_bottom, priority, core_id, std::addressof(process)));
            }

            /* Commit the thread reservation. */
            thread_reservation.Commit();

            /* Clone the current fpu status to the new thread. */
            thread->GetContext().CloneFpuStatus();

            /* Register the new thread. */
            R_TRY(KThread::Register(thread.GetPointerUnsafe()));

            /* Add the thread to the handle table. */
            R_TRY(process.GetHandleTable().Add(out, thread.GetPointerUnsafe()));

            return ResultSuccess();
        }

        Result StartThread(ams::svc::Handle thread_handle) {
            /* Get the thread from its handle. */
            KScopedAutoObject thread = GetCurrentProcess().GetHandleTable().GetObject<KThread>(thread_handle);
            R_UNLESS(thread.IsNotNull(), svc::ResultInvalidHandle());

            /* Try to start the thread. */
            R_TRY(thread->Run());

            /* If we succeeded, persist a reference to the thread. */
            thread->Open();
            return ResultSuccess();
        }

        void ExitThread() {
            GetCurrentThread().Exit();
            MESOSPHERE_PANIC("Process survived call to exit");
        }

        void SleepThread(int64_t ns) {
            /* When the input tick is positive, sleep. */
            if (AMS_LIKELY(ns > 0)) {
                /* Convert the timeout from nanoseconds to ticks. */
                /* NOTE: Nintendo does not use this conversion logic in WaitSynchronization... */
                s64 timeout;

                const ams::svc::Tick offset_tick(TimeSpan::FromNanoSeconds(ns));
                if (AMS_LIKELY(offset_tick > 0)) {
                    timeout = KHardwareTimer::GetTick() + offset_tick + 2;
                    if (AMS_UNLIKELY(timeout <= 0)) {
                        timeout = std::numeric_limits<s64>::max();
                    }
                } else {
                    timeout = std::numeric_limits<s64>::max();
                }

                /* Sleep. */
                /* NOTE: Nintendo does not check the result of this sleep. */
                GetCurrentThread().Sleep(timeout);
            } else if (ns == ams::svc::YieldType_WithoutCoreMigration) {
                KScheduler::YieldWithoutCoreMigration();
            } else if (ns == ams::svc::YieldType_WithCoreMigration) {
                KScheduler::YieldWithCoreMigration();
            } else if (ns == ams::svc::YieldType_ToAnyThread) {
                KScheduler::YieldToAnyThread();
            } else {
                /* Nintendo does nothing at all if an otherwise invalid value is passed. */
            }
        }

        Result GetThreadPriority(int32_t *out_priority, ams::svc::Handle thread_handle) {
            /* Get the thread from its handle. */
            KScopedAutoObject thread = GetCurrentProcess().GetHandleTable().GetObject<KThread>(thread_handle);
            R_UNLESS(thread.IsNotNull(), svc::ResultInvalidHandle());

            /* Get the thread's priority. */
            *out_priority = thread->GetPriority();
            return ResultSuccess();
        }

        Result GetThreadId(uint64_t *out_thread_id, ams::svc::Handle thread_handle) {
            /* Get the thread from its handle. */
            KScopedAutoObject thread = GetCurrentProcess().GetHandleTable().GetObject<KThread>(thread_handle);
            R_UNLESS(thread.IsNotNull(), svc::ResultInvalidHandle());

            /* Get the thread's id. */
            *out_thread_id = thread->GetId();
            return ResultSuccess();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result CreateThread64(ams::svc::Handle *out_handle, ams::svc::ThreadFunc func, ams::svc::Address arg, ams::svc::Address stack_bottom, int32_t priority, int32_t core_id) {
        return CreateThread(out_handle, func, arg, stack_bottom, priority, core_id);
    }

    Result StartThread64(ams::svc::Handle thread_handle) {
        return StartThread(thread_handle);
    }

    void ExitThread64() {
        return ExitThread();
    }

    void SleepThread64(int64_t ns) {
        return SleepThread(ns);
    }

    Result GetThreadPriority64(int32_t *out_priority, ams::svc::Handle thread_handle) {
        return GetThreadPriority(out_priority, thread_handle);
    }

    Result SetThreadPriority64(ams::svc::Handle thread_handle, int32_t priority) {
        MESOSPHERE_PANIC("Stubbed SvcSetThreadPriority64 was called.");
    }

    Result GetThreadCoreMask64(int32_t *out_core_id, uint64_t *out_affinity_mask, ams::svc::Handle thread_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetThreadCoreMask64 was called.");
    }

    Result SetThreadCoreMask64(ams::svc::Handle thread_handle, int32_t core_id, uint64_t affinity_mask) {
        MESOSPHERE_PANIC("Stubbed SvcSetThreadCoreMask64 was called.");
    }

    Result GetThreadId64(uint64_t *out_thread_id, ams::svc::Handle thread_handle) {
        return GetThreadId(out_thread_id, thread_handle);
    }

    Result GetDebugFutureThreadInfo64(ams::svc::lp64::LastThreadContext *out_context, uint64_t *thread_id, ams::svc::Handle debug_handle, int64_t ns) {
        MESOSPHERE_PANIC("Stubbed SvcGetDebugFutureThreadInfo64 was called.");
    }

    Result GetLastThreadInfo64(ams::svc::lp64::LastThreadContext *out_context, ams::svc::Address *out_tls_address, uint32_t *out_flags) {
        MESOSPHERE_PANIC("Stubbed SvcGetLastThreadInfo64 was called.");
    }

    Result GetThreadContext364(KUserPointer<ams::svc::ThreadContext *> out_context, ams::svc::Handle thread_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetThreadContext364 was called.");
    }

    Result GetThreadList64(int32_t *out_num_threads, KUserPointer<uint64_t *> out_thread_ids, int32_t max_out_count, ams::svc::Handle debug_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetThreadList64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    Result CreateThread64From32(ams::svc::Handle *out_handle, ams::svc::ThreadFunc func, ams::svc::Address arg, ams::svc::Address stack_bottom, int32_t priority, int32_t core_id) {
        return CreateThread(out_handle, func, arg, stack_bottom, priority, core_id);
    }

    Result StartThread64From32(ams::svc::Handle thread_handle) {
        return StartThread(thread_handle);
    }

    void ExitThread64From32() {
        return ExitThread();
    }

    void SleepThread64From32(int64_t ns) {
        return SleepThread(ns);
    }

    Result GetThreadPriority64From32(int32_t *out_priority, ams::svc::Handle thread_handle) {
        return GetThreadPriority(out_priority, thread_handle);
    }

    Result SetThreadPriority64From32(ams::svc::Handle thread_handle, int32_t priority) {
        MESOSPHERE_PANIC("Stubbed SvcSetThreadPriority64From32 was called.");
    }

    Result GetThreadCoreMask64From32(int32_t *out_core_id, uint64_t *out_affinity_mask, ams::svc::Handle thread_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetThreadCoreMask64From32 was called.");
    }

    Result SetThreadCoreMask64From32(ams::svc::Handle thread_handle, int32_t core_id, uint64_t affinity_mask) {
        MESOSPHERE_PANIC("Stubbed SvcSetThreadCoreMask64From32 was called.");
    }

    Result GetThreadId64From32(uint64_t *out_thread_id, ams::svc::Handle thread_handle) {
        return GetThreadId(out_thread_id, thread_handle);
    }

    Result GetDebugFutureThreadInfo64From32(ams::svc::ilp32::LastThreadContext *out_context, uint64_t *thread_id, ams::svc::Handle debug_handle, int64_t ns) {
        MESOSPHERE_PANIC("Stubbed SvcGetDebugFutureThreadInfo64From32 was called.");
    }

    Result GetLastThreadInfo64From32(ams::svc::ilp32::LastThreadContext *out_context, ams::svc::Address *out_tls_address, uint32_t *out_flags) {
        MESOSPHERE_PANIC("Stubbed SvcGetLastThreadInfo64From32 was called.");
    }

    Result GetThreadContext364From32(KUserPointer<ams::svc::ThreadContext *> out_context, ams::svc::Handle thread_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetThreadContext364From32 was called.");
    }

    Result GetThreadList64From32(int32_t *out_num_threads, KUserPointer<uint64_t *> out_thread_ids, int32_t max_out_count, ams::svc::Handle debug_handle) {
        MESOSPHERE_PANIC("Stubbed SvcGetThreadList64From32 was called.");
    }

}
