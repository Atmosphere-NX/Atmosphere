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

#pragma once
#include <vapours.hpp>
#include <stratosphere/os/os_thread_common.hpp>

namespace ams::os {

    struct ThreadType;
    struct MultiWaitHolderType;

    Result CreateThread(ThreadType *thread, ThreadFunction function, void *argument, void *stack, size_t stack_size, s32 priority, s32 ideal_core);
    Result CreateThread(ThreadType *thread, ThreadFunction function, void *argument, void *stack, size_t stack_size, s32 priority);
    void DestroyThread(ThreadType *thread);
    void StartThread(ThreadType *thread);

    ThreadType *GetCurrentThread();

    void WaitThread(ThreadType *thread);
    bool TryWaitThread(ThreadType *thread);

    void YieldThread();
    void SleepThread(TimeSpan time);

    s32 SuspendThread(ThreadType *thread);
    s32 ResumeThread(ThreadType *thread);
    s32 GetThreadSuspendCount(const ThreadType *thread);

    void CancelThreadSynchronization(ThreadType *Thread);

    /* TODO: void GetThreadContext(ThreadContextInfo *out_context, const ThreadType *thread); */

    s32 ChangeThreadPriority(ThreadType *thread, s32 priority);
    s32 GetThreadPriority(const ThreadType *thread);
    s32 GetThreadCurrentPriority(const ThreadType *thread);

    void SetThreadName(ThreadType *thread, const char *name);
    void SetThreadNamePointer(ThreadType *thread, const char *name);
    const char *GetThreadNamePointer(const ThreadType *thread);

    s32 GetCurrentProcessorNumber();
    s32 GetCurrentCoreNumber();

    void SetThreadCoreMask(ThreadType *thread, s32 ideal_core, u64 affinity_mask);
    void GetThreadCoreMask(s32 *out_ideal_core, u64 *out_affinity_mask, const ThreadType *thread);

    u64 GetThreadAvailableCoreMask();

    ThreadId GetThreadId(const ThreadType *thread);

    void InitializeMultiWaitHolder(MultiWaitHolderType *holder, ThreadType *thread);

}
