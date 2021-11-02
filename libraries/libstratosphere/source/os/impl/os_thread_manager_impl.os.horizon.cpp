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
#include <stratosphere.hpp>
#include "os_thread_manager_impl.os.horizon.hpp"
#include "os_thread_manager.hpp"

namespace ams::os::impl {

    thread_local ThreadType *g_current_thread_pointer;

    namespace {

        s32 ConvertToHorizonPriority(s32 user_priority) {
            const s32 horizon_priority = user_priority + UserThreadPriorityOffset;
            AMS_ASSERT(HighestTargetThreadPriority <= horizon_priority && horizon_priority <= LowestTargetThreadPriority);
            return horizon_priority;
        }

        s32 ConvertToUserPriority(s32 horizon_priority) {
            AMS_ASSERT(HighestTargetThreadPriority <= horizon_priority && horizon_priority <= LowestTargetThreadPriority);
            return horizon_priority - UserThreadPriorityOffset;
        }

        void InvokeThread(uintptr_t _thread) {
            ThreadType *thread = reinterpret_cast<ThreadType *>(_thread);

            /* Set the thread's id. */
            u64 thread_id;
            R_ABORT_UNLESS(svc::GetThreadId(std::addressof(thread_id), thread->thread_impl->handle));
            thread->thread_id = thread_id;

            /* Invoke the thread. */
            ThreadManager::InvokeThread(thread);
        }

    }

    ThreadManagerHorizonImpl::ThreadManagerHorizonImpl(ThreadType *main_thread) {
        /* Get the thread impl object from libnx. */
        ThreadImpl *thread_impl = ::threadGetSelf();

        /* Hack around libnx's main thread, to ensure stratosphere thread type consistency. */
        {
            auto *tlr = reinterpret_cast<uintptr_t *>(svc::GetThreadLocalRegion());
            for (size_t i = sizeof(svc::ThreadLocalRegion) / sizeof(uintptr_t); i > 0; --i) {
                if (auto *candidate = reinterpret_cast<ThreadImpl *>(tlr[i - 1]); candidate == thread_impl) {
                    ThreadImpl *embedded_thread = std::addressof(main_thread->thread_impl_storage);

                    *embedded_thread = *thread_impl;

                    if (embedded_thread->next) {
                        embedded_thread->next->prev_next = std::addressof(embedded_thread->next);
                    }

                    thread_impl = embedded_thread;
                    tlr[i-1]    = reinterpret_cast<uintptr_t>(thread_impl);
                    break;
                }
            }
        }

        /* Get the thread priority. */
        s32 horizon_priority;
        R_ABORT_UNLESS(svc::GetThreadPriority(std::addressof(horizon_priority), thread_impl->handle));

        SetupThreadObjectUnsafe(main_thread, thread_impl, nullptr, nullptr, thread_impl->stack_mirror, thread_impl->stack_sz, ConvertToUserPriority(horizon_priority));

        /* Set the thread id. */
        u64 thread_id;
        R_ABORT_UNLESS(svc::GetThreadId(std::addressof(thread_id), thread_impl->handle));
        main_thread->thread_id = thread_id;

        /* NOTE: Here Nintendo would set the thread pointer in TLS. */
    }

    Result ThreadManagerHorizonImpl::CreateThread(ThreadType *thread, s32 ideal_core) {
        /* Note: Here Nintendo would set the stack args to point to ThreadManager::InvokeThread. */

        s32 count = 0;
        while (true) {
            R_TRY_CATCH(::threadCreate(thread->thread_impl, reinterpret_cast<::ThreadFunc>(reinterpret_cast<void *>(&InvokeThread)), thread, thread->stack, thread->stack_size, ConvertToHorizonPriority(thread->base_priority), ideal_core)) {
                R_CATCH(svc::ResultOutOfResource) {
                    if ((++count) < 10) {
                        os::SleepThread(TimeSpan::FromMilliSeconds(10));
                        continue;
                    }
                    return os::ResultOutOfResource();
                }
            } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

            return ResultSuccess();
        }
    }

    void ThreadManagerHorizonImpl::DestroyThreadUnsafe(ThreadType *thread) {
        R_ABORT_UNLESS(::threadClose(thread->thread_impl));
    }

    void ThreadManagerHorizonImpl::StartThread(const ThreadType *thread) {
        R_ABORT_UNLESS(::threadStart(thread->thread_impl));
    }

    void ThreadManagerHorizonImpl::WaitForThreadExit(ThreadType *thread) {
        const svc::Handle handle = thread->thread_impl->handle;

        while (true) {
            s32 index;
            R_TRY_CATCH(svc::WaitSynchronization(std::addressof(index), std::addressof(handle), 1, svc::WaitInfinite)) {
                R_CATCH(svc::ResultCancelled) { continue; }
            } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

            return;
        }
    }

    bool ThreadManagerHorizonImpl::TryWaitForThreadExit(ThreadType *thread) {
        const svc::Handle handle = thread->thread_impl->handle;

        while (true) {
            /* Continuously wait, until success or timeout. */
            s32 index;
            Result res = svc::WaitSynchronization(std::addressof(index), std::addressof(handle), 1, 0);

            /* If we succeeded, we're signaled. */
            if (R_SUCCEEDED(res)) {
                return true;
            }

            /* If we timed out, we're not signaled. */
            if (svc::ResultTimedOut::Includes(res)) {
                return false;
            }

            AMS_ABORT_UNLESS(svc::ResultCancelled::Includes(res));
        }
    }

    void ThreadManagerHorizonImpl::YieldThread() {
        if (hos::GetVersion() >= hos::Version_4_0_0) {
            svc::SleepThread(svc::YieldType_WithCoreMigration);
        } else {
            svc::SleepThread(svc::YieldType_WithoutCoreMigration);
        }
    }

    bool ThreadManagerHorizonImpl::ChangePriority(ThreadType *thread, s32 priority) {
        auto res = svc::SetThreadPriority(thread->thread_impl->handle, ConvertToHorizonPriority(priority));
        if (svc::ResultInvalidPriority::Includes(res)) {
            AMS_ABORT("Invalid thread priority");
        }

        return R_SUCCEEDED(res);
    }

    s32 ThreadManagerHorizonImpl::GetCurrentPriority(const ThreadType *thread) const {
        s32 priority;
        R_ABORT_UNLESS(svc::GetThreadPriority(std::addressof(priority), thread->thread_impl->handle));

        return ConvertToUserPriority(priority);
    }

    ThreadId ThreadManagerHorizonImpl::GetThreadId(const ThreadType *thread) const {
        return thread->thread_id;
    }

    void ThreadManagerHorizonImpl::SuspendThreadUnsafe(ThreadType *thread) {
        R_ABORT_UNLESS(svc::SetThreadActivity(thread->thread_impl->handle, svc::ThreadActivity_Paused));
    }

    void ThreadManagerHorizonImpl::ResumeThreadUnsafe(ThreadType *thread) {
        R_ABORT_UNLESS(svc::SetThreadActivity(thread->thread_impl->handle, svc::ThreadActivity_Runnable));
    }

    void ThreadManagerHorizonImpl::CancelThreadSynchronizationUnsafe(ThreadType *thread) {
        R_ABORT_UNLESS(svc::CancelSynchronization(thread->thread_impl->handle));
    }

    /* TODO: void GetThreadContextUnsafe(ThreadContextInfo *out_context, const ThreadType *thread); */

    s32 ThreadManagerHorizonImpl::GetCurrentCoreNumber() const {
        return svc::GetCurrentProcessorNumber();
    }

    void ThreadManagerHorizonImpl::SetThreadCoreMask(ThreadType *thread, s32 ideal_core, u64 affinity_mask) const {
        R_ABORT_UNLESS(svc::SetThreadCoreMask(thread->thread_impl->handle, ideal_core, affinity_mask));
    }

    void ThreadManagerHorizonImpl::GetThreadCoreMask(s32 *out_ideal_core, u64 *out_affinity_mask, const ThreadType *thread) const {
        s32 ideal_core;
        u64 affinity_mask;

        R_ABORT_UNLESS(svc::GetThreadCoreMask(std::addressof(ideal_core), std::addressof(affinity_mask), thread->thread_impl->handle));

        if (out_ideal_core) {
            *out_ideal_core = ideal_core;
        }
        if (out_affinity_mask) {
            *out_affinity_mask = affinity_mask;
        }
    }

    u64 ThreadManagerHorizonImpl::GetThreadAvailableCoreMask() const {
        u64 core_mask;
        R_ABORT_UNLESS(svc::GetInfo(std::addressof(core_mask), svc::InfoType_CoreMask, svc::PseudoHandle::CurrentProcess, 0));
        return core_mask;
    }


}
