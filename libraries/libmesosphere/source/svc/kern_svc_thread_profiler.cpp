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

        Result GetLastThreadInfoImpl(ams::svc::LastThreadContext *out_context, uintptr_t *out_tls_address, uint32_t *out_flags) {
            /* Disable interrupts. */
            KScopedInterruptDisable di;

            /* Get the previous thread. */
            KThread *prev_thread = Kernel::GetScheduler().GetPreviousThread();
            R_UNLESS(prev_thread != nullptr, svc::ResultNoThread());

            /* Verify the last thread was owned by the current process. */
            R_UNLESS(prev_thread->GetOwnerProcess() == GetCurrentProcessPointer(), svc::ResultUnknownThread());

            /* Clear the output flags. */
            *out_flags = 0;

            /* Get the thread's exception context. */
            GetExceptionContext(prev_thread)->GetSvcThreadContext(out_context);

            /* Get the tls address. */
            *out_tls_address = GetInteger(prev_thread->GetThreadLocalRegionAddress());

            /* Set the syscall flag if appropriate. */
            if (prev_thread->IsCallingSvc()) {
                *out_flags |= ams::svc::LastThreadInfoFlag_ThreadInSystemCall;
            }

            return ResultSuccess();
        }

        Result SynchronizeCurrentProcessToFutureTime(int64_t ns) {
            /* Get the wait object. */
            KWaitObject *wait_object = GetCurrentProcess().GetWaitObjectPointer();

            /* Convert the timeout from nanoseconds to ticks. */
            s64 timeout;
            if (ns > 0) {
                u64 ticks = KHardwareTimer::GetTick();
                ticks += ams::svc::Tick(TimeSpan::FromNanoSeconds(ns));
                ticks += 2;

                timeout = ticks;
            } else {
                timeout = ns;
            }

            /* Synchronize to the desired time. */
            R_TRY(wait_object->Synchronize(timeout));

            return ResultSuccess();
        }

        Result GetDebugFutureThreadInfo(ams::svc::LastThreadContext *out_context, uint64_t *out_thread_id, ams::svc::Handle debug_handle, int64_t ns) {
            /* Only allow invoking the svc on development hardware. */
            R_UNLESS(KTargetSystem::IsDebugMode(), svc::ResultNoThread());

            /* Get the debug object. */
            KScopedAutoObject debug = GetCurrentProcess().GetHandleTable().GetObject<KDebug>(debug_handle);
            R_UNLESS(debug.IsNotNull(), svc::ResultInvalidHandle());

            /* Synchronize the current process to the desired time. */
            R_TRY(SynchronizeCurrentProcessToFutureTime(ns));

            /* Get the running thread info. */
            R_TRY(debug->GetRunningThreadInfo(out_context, out_thread_id));

            return ResultSuccess();
        }

        Result LegacyGetFutureThreadInfo(ams::svc::LastThreadContext *out_context, uintptr_t *out_tls_address, uint32_t *out_flags, int64_t ns) {
            /* Only allow invoking the svc on development hardware. */
            R_UNLESS(KTargetSystem::IsDebugMode(), svc::ResultNoThread());

            /* Synchronize the current process to the desired time. */
            R_TRY(SynchronizeCurrentProcessToFutureTime(ns));

            /* Get the thread info. */
            R_TRY(GetLastThreadInfoImpl(out_context, out_tls_address, out_flags));

            return ResultSuccess();
        }

        Result GetLastThreadInfo(ams::svc::LastThreadContext *out_context, uintptr_t *out_tls_address, uint32_t *out_flags) {
            /* Only allow invoking the svc on development hardware. */
            R_UNLESS(KTargetSystem::IsDebugMode(), svc::ResultNoThread());

            /* Get the thread info. */
            R_TRY(GetLastThreadInfoImpl(out_context, out_tls_address, out_flags));

            return ResultSuccess();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result GetDebugFutureThreadInfo64(ams::svc::lp64::LastThreadContext *out_context, uint64_t *out_thread_id, ams::svc::Handle debug_handle, int64_t ns) {
        return GetDebugFutureThreadInfo(out_context, out_thread_id, debug_handle, ns);
    }

    Result LegacyGetFutureThreadInfo64(ams::svc::lp64::LastThreadContext *out_context, ams::svc::Address *out_tls_address, uint32_t *out_flags, int64_t ns) {
        return LegacyGetFutureThreadInfo(out_context, reinterpret_cast<uintptr_t *>(out_tls_address), out_flags, ns);
    }

    Result GetLastThreadInfo64(ams::svc::lp64::LastThreadContext *out_context, ams::svc::Address *out_tls_address, uint32_t *out_flags) {
        static_assert(sizeof(*out_tls_address) == sizeof(uintptr_t));
        return GetLastThreadInfo(out_context, reinterpret_cast<uintptr_t *>(out_tls_address), out_flags);
    }

    /* ============================= 64From32 ABI ============================= */

    Result GetDebugFutureThreadInfo64From32(ams::svc::ilp32::LastThreadContext *out_context, uint64_t *out_thread_id, ams::svc::Handle debug_handle, int64_t ns) {
        ams::svc::LastThreadContext context = {};
        R_TRY(GetDebugFutureThreadInfo(std::addressof(context), out_thread_id, debug_handle, ns));

        *out_context = {
            .fp = static_cast<u32>(context.fp),
            .sp = static_cast<u32>(context.sp),
            .lr = static_cast<u32>(context.lr),
            .pc = static_cast<u32>(context.pc),
        };
        return ResultSuccess();
    }

    Result LegacyGetFutureThreadInfo64From32(ams::svc::ilp32::LastThreadContext *out_context, ams::svc::Address *out_tls_address, uint32_t *out_flags, int64_t ns) {
        static_assert(sizeof(*out_tls_address) == sizeof(uintptr_t));

        ams::svc::LastThreadContext context = {};
        R_TRY(LegacyGetFutureThreadInfo(std::addressof(context), reinterpret_cast<uintptr_t *>(out_tls_address), out_flags, ns));

        *out_context = {
            .fp = static_cast<u32>(context.fp),
            .sp = static_cast<u32>(context.sp),
            .lr = static_cast<u32>(context.lr),
            .pc = static_cast<u32>(context.pc),
        };
        return ResultSuccess();
    }

    Result GetLastThreadInfo64From32(ams::svc::ilp32::LastThreadContext *out_context, ams::svc::Address *out_tls_address, uint32_t *out_flags) {
        static_assert(sizeof(*out_tls_address) == sizeof(uintptr_t));

        ams::svc::LastThreadContext context = {};
        R_TRY(GetLastThreadInfo(std::addressof(context), reinterpret_cast<uintptr_t *>(out_tls_address), out_flags));

        *out_context = {
            .fp = static_cast<u32>(context.fp),
            .sp = static_cast<u32>(context.sp),
            .lr = static_cast<u32>(context.lr),
            .pc = static_cast<u32>(context.pc),
        };
        return ResultSuccess();
    }

}
