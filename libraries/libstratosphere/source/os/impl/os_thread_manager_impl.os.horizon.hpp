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
#pragma once
#include <stratosphere.hpp>

namespace ams::os::impl {

    constexpr inline s32 TargetThreadPriorityRangeSize   = svc::LowestThreadPriority - svc::HighestThreadPriority + 1;
    constexpr inline s32 ReservedThreadPriorityRangeSize = svc::SystemThreadPriorityHighest;
    constexpr inline s32 SystemThreadPriorityRangeSize   = TargetThreadPriorityRangeSize - ReservedThreadPriorityRangeSize;
    constexpr inline s32 UserThreadPriorityOffset        = 28;

    constexpr inline s32 HighestTargetThreadPriority = 0;
    constexpr inline s32 LowestTargetThreadPriority  = TargetThreadPriorityRangeSize - 1;

    extern thread_local ThreadType *g_current_thread_pointer;

    class ThreadManagerHorizonImpl {
        NON_COPYABLE(ThreadManagerHorizonImpl);
        NON_MOVEABLE(ThreadManagerHorizonImpl);
        public:
            explicit ThreadManagerHorizonImpl(ThreadType *main_thread);

            Result CreateThread(ThreadType *thread, s32 ideal_core);
            void DestroyThreadUnsafe(ThreadType *thread);
            void StartThread(const ThreadType *thread);
            void WaitForThreadExit(ThreadType *thread);
            bool TryWaitForThreadExit(ThreadType *thread);
            void YieldThread();
            bool ChangePriority(ThreadType *thread, s32 priority);
            s32 GetCurrentPriority(const ThreadType *thread) const;
            ThreadId GetThreadId(const ThreadType *thread) const;

            void SuspendThreadUnsafe(ThreadType *thread);
            void ResumeThreadUnsafe(ThreadType *thread);

            void CancelThreadSynchronizationUnsafe(ThreadType *thread);

            /* TODO: void GetThreadContextUnsafe(ThreadContextInfo *out_context, const ThreadType *thread); */

            void NotifyThreadNameChangedImpl(const ThreadType *thread) const { /* ... */ }

            void SetCurrentThread(ThreadType *thread) const {
                g_current_thread_pointer = thread;
            }

            ThreadType *GetCurrentThread() const {
                return g_current_thread_pointer;
            }

            s32 GetCurrentCoreNumber() const;
            s32 GetDefaultCoreNumber() const { return svc::IdealCoreUseProcessValue; }

            void SetThreadCoreMask(ThreadType *thread, s32 ideal_core, u64 affinity_mask) const;
            void GetThreadCoreMask(s32 *out_ideal_core, u64 *out_affinity_mask, const ThreadType *thread) const;
            u64 GetThreadAvailableCoreMask() const;

            NORETURN void ExitProcessImpl() {
                svc::ExitProcess();
                AMS_ABORT("Process was exited");
            }
    };

    using ThreadManagerImpl = ThreadManagerHorizonImpl;

}
