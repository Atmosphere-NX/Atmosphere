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
#include "impl/os_thread_manager.hpp"
#include "impl/os_timeout_helper.hpp"
#include "impl/os_multiple_wait_holder_impl.hpp"

namespace ams::os {

    namespace {

        size_t CheckThreadNameLength(const char *name) {
            const char *cur = name;
            for (size_t len = 0; len < ThreadNameLengthMax; ++len) {
                if (*(cur++) == 0) {
                    return len;
                }
            }

            AMS_ABORT("ThreadNameLength too large");
        }

        void ValidateThreadArguments(ThreadType *thread, void *stack, size_t stack_size, s32 priority) {
            AMS_ASSERT(stack != nullptr);
            AMS_ASSERT(util::IsAligned(reinterpret_cast<uintptr_t>(stack), ThreadStackAlignment));
            AMS_ASSERT(stack_size > 0);
            AMS_ASSERT(util::IsAligned(stack_size, ThreadStackAlignment));

            AMS_UNUSED(thread, stack, stack_size, priority);
        }

    }

    Result CreateThread(ThreadType *thread, ThreadFunction function, void *argument, void *stack, size_t stack_size, s32 priority, s32 ideal_core) {
        ValidateThreadArguments(thread, stack, stack_size, priority);
        AMS_ASSERT(GetThreadAvailableCoreMask() & (1ul << ideal_core));
        return impl::GetThreadManager().CreateThread(thread, function, argument, stack, stack_size, priority, ideal_core);
    }

    Result CreateThread(ThreadType *thread, ThreadFunction function, void *argument, void *stack, size_t stack_size, s32 priority) {
        ValidateThreadArguments(thread, stack, stack_size, priority);
        return impl::GetThreadManager().CreateThread(thread, function, argument, stack, stack_size, priority);
    }

    void DestroyThread(ThreadType *thread) {
        auto &manager = impl::GetThreadManager();

        AMS_ASSERT(thread->state == ThreadType::State_Initialized || thread->state == ThreadType::State_Started || thread->state == ThreadType::State_Terminated);
        AMS_ASSERT(thread != manager.GetMainThread());

        manager.DestroyThread(thread);
    }

    void StartThread(ThreadType *thread) {
        AMS_ASSERT(thread->state == ThreadType::State_Initialized);
        impl::GetThreadManager().StartThread(thread);
    }

    ThreadType *GetCurrentThread() {
        return impl::GetCurrentThread();
    }

    void WaitThread(ThreadType *thread) {
        AMS_ASSERT(thread != impl::GetCurrentThread());
        AMS_ASSERT(thread->state == ThreadType::State_Initialized || thread->state == ThreadType::State_Started || thread->state == ThreadType::State_Terminated);

        return impl::GetThreadManager().WaitThread(thread);
    }

    bool TryWaitThread(ThreadType *thread) {
        AMS_ASSERT(thread->state == ThreadType::State_Initialized || thread->state == ThreadType::State_Started || thread->state == ThreadType::State_Terminated);
        return impl::GetThreadManager().TryWaitThread(thread);
    }

    void YieldThread() {
        return impl::GetThreadManager().YieldThread();
    }

    void SleepThread(TimeSpan time) {
        impl::TimeoutHelper::Sleep(time);
    }

    s32 SuspendThread(ThreadType *thread) {
        AMS_ASSERT(thread->state == ThreadType::State_Started);
        AMS_ASSERT(thread != impl::GetCurrentThread());

        return impl::GetThreadManager().SuspendThread(thread);
    }

    s32 ResumeThread(ThreadType *thread) {
        AMS_ASSERT(thread->state == ThreadType::State_Started);

        return impl::GetThreadManager().ResumeThread(thread);
    }

    s32 GetThreadSuspendCount(const ThreadType *thread) {
        return thread->suspend_count;
    }

    void CancelThreadSynchronization(ThreadType *thread) {
        AMS_ASSERT(thread->state == ThreadType::State_Started || thread->state == ThreadType::State_Terminated);

        return impl::GetThreadManager().CancelThreadSynchronization(thread);
    }

    /* TODO: void GetThreadContext(ThreadContextInfo *out_context, const ThreadType *thread); */

    s32 ChangeThreadPriority(ThreadType *thread, s32 priority) {
        AMS_ASSERT(thread->state == ThreadType::State_Initialized || thread->state == ThreadType::State_DestroyedBeforeStarted || thread->state == ThreadType::State_Started || thread->state == ThreadType::State_Terminated);
        {
            std::scoped_lock lk(GetReference(thread->cs_thread));

            const s32 prev_prio = thread->base_priority;

            const bool success = impl::GetThreadManager().ChangePriority(thread, priority);
            AMS_ASSERT(success);
            AMS_UNUSED(success);

            thread->base_priority = priority;

            return prev_prio;
        }
    }

    s32 GetThreadPriority(const ThreadType *thread) {
        AMS_ASSERT(thread->state == ThreadType::State_Initialized || thread->state == ThreadType::State_DestroyedBeforeStarted || thread->state == ThreadType::State_Started || thread->state == ThreadType::State_Terminated);
        return thread->base_priority;
    }

    s32 GetThreadCurrentPriority(const ThreadType *thread) {
        AMS_ASSERT(thread->state == ThreadType::State_Initialized || thread->state == ThreadType::State_DestroyedBeforeStarted || thread->state == ThreadType::State_Started || thread->state == ThreadType::State_Terminated);
        return impl::GetThreadManager().GetCurrentPriority(thread);
    }

    void SetThreadName(ThreadType *thread, const char *name) {
        AMS_ASSERT(thread->state == ThreadType::State_Initialized || thread->state == ThreadType::State_DestroyedBeforeStarted || thread->state == ThreadType::State_Started || thread->state == ThreadType::State_Terminated);

        if (name == nullptr) {
            impl::GetThreadManager().SetInitialThreadNameUnsafe(thread);
            return;
        }

        const size_t name_size = CheckThreadNameLength(name) + 1;
        std::memcpy(thread->name_buffer, name, name_size);
        SetThreadNamePointer(thread, thread->name_buffer);
    }

    void SetThreadNamePointer(ThreadType *thread, const char *name) {
        AMS_ASSERT(thread->state == ThreadType::State_Initialized || thread->state == ThreadType::State_DestroyedBeforeStarted || thread->state == ThreadType::State_Started || thread->state == ThreadType::State_Terminated);

        if (name == nullptr) {
            impl::GetThreadManager().SetInitialThreadNameUnsafe(thread);
            return;
        }

        thread->name_pointer = name;
        impl::GetThreadManager().NotifyThreadNameChanged(thread);
    }

    const char *GetThreadNamePointer(const ThreadType *thread) {
        AMS_ASSERT(thread->state == ThreadType::State_Initialized || thread->state == ThreadType::State_DestroyedBeforeStarted || thread->state == ThreadType::State_Started || thread->state == ThreadType::State_Terminated);

        return thread->name_pointer;
    }

    s32 GetCurrentCoreNumber() {
        return impl::GetThreadManager().GetCurrentCoreNumber();
    }

    s32 GetCurrentProcessorNumber() {
        return GetCurrentCoreNumber();
    }

    void SetThreadCoreMask(ThreadType *thread, s32 ideal_core, u64 affinity_mask) {
        AMS_ASSERT(ideal_core == IdealCoreDontCare || ideal_core == IdealCoreUseDefault || ideal_core == IdealCoreNoUpdate || (0 <= ideal_core && ideal_core < impl::CoreAffinityMaskBitWidth));
        if (ideal_core != IdealCoreUseDefault) {
            AMS_ASSERT(affinity_mask != 0);
            AMS_ASSERT((affinity_mask & ~GetThreadAvailableCoreMask()) == 0);
        }
        if (ideal_core >= 0) {
            AMS_ASSERT((affinity_mask & (1ul << ideal_core)) != 0);
        }

        return impl::GetThreadManager().SetThreadCoreMask(thread, ideal_core, affinity_mask);
    }

    void GetThreadCoreMask(s32 *out_ideal_core, u64 *out_affinity_mask, const ThreadType *thread) {
        return impl::GetThreadManager().GetThreadCoreMask(out_ideal_core, out_affinity_mask, thread);
    }

    u64 GetThreadAvailableCoreMask() {
        return impl::GetThreadManager().GetThreadAvailableCoreMask();
    }

    ThreadId GetThreadId(const ThreadType *thread) {
        return impl::GetThreadManager().GetThreadId(thread);
    }

}
