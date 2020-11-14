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
#include <stratosphere.hpp>
#include "os_thread_manager.hpp"
#include "os_waitable_manager_impl.hpp"
#include "os_waitable_holder_base.hpp"
#include "os_waitable_holder_impl.hpp"
#include "os_waitable_object_list.hpp"

namespace ams::os::impl {

    void SetupThreadObjectUnsafe(ThreadType *thread, ThreadImpl *thread_impl, ThreadFunction function, void *arg, void *stack, size_t stack_size, s32 priority) {
        /* Setup objects. */
        new (GetPointer(thread->cs_thread)) impl::InternalCriticalSection;
        new (GetPointer(thread->cv_thread)) impl::InternalConditionVariable;

        new (GetPointer(thread->all_threads_node)) util::IntrusiveListNode;
        new (GetPointer(thread->waitlist))         WaitableObjectList;

        /* Set member variables. */
        thread->thread_impl    = (thread_impl != nullptr) ? thread_impl : std::addressof(thread->thread_impl_storage);
        thread->function       = function;
        thread->argument       = arg;
        thread->stack          = stack;
        thread->stack_size     = stack_size;
        thread->base_priority  = priority;
        thread->suspend_count  = 0;
        thread->name_buffer[0] = '\x00';
        thread->name_pointer   = thread->name_buffer;

        /* Set internal tls variables. */
        thread->atomic_sf_inline_context = 0;

        /* Mark initialized. */
        thread->state = ThreadType::State_Initialized;
    }

    void ThreadManager::InvokeThread(ThreadType *thread) {
        auto &manager = GetThreadManager();

        manager.SetCurrentThread(thread);
        manager.NotifyThreadNameChanged(thread);

        {
            std::unique_lock lk(GetReference(thread->cs_thread));
            while (thread->state == ThreadType::State_Initialized) {
                GetReference(thread->cv_thread).Wait(GetPointer(thread->cs_thread));
            }

            if (thread->state == ThreadType::State_Started) {
                lk.unlock();

                thread->function(thread->argument);
            }
        }

        manager.CleanupThread();
    }

    ThreadManager::ThreadManager() : impl(std::addressof(main_thread)), total_thread_stack_size(0), num_created_threads(0) {
        this->main_thread.state = ThreadType::State_Started;

        this->SetCurrentThread(std::addressof(this->main_thread));

        this->PlaceThreadObjectUnderThreadManagerSafe(std::addressof(this->main_thread));
    }

    void ThreadManager::CleanupThread() {
        ThreadType *thread = this->GetCurrentThread();

        {
            std::scoped_lock lk(GetReference(thread->cs_thread));

            thread->state = ThreadType::State_Terminated;

            GetReference(thread->cv_thread).Broadcast();
            GetReference(thread->waitlist).SignalAllThreads();
        }
    }

    Result ThreadManager::CreateThread(ThreadType *thread, ThreadFunction function, void *argument, void *stack, size_t stack_size, s32 priority, s32 ideal_core) {
        SetupThreadObjectUnsafe(thread, nullptr, function, argument, stack, stack_size, priority);

        auto guard = SCOPE_GUARD { thread->state = ThreadType::State_NotInitialized; };
        R_TRY(this->impl.CreateThread(thread, ideal_core));
        guard.Cancel();

        this->PlaceThreadObjectUnderThreadManagerSafe(thread);

        return ResultSuccess();
    }

    Result ThreadManager::CreateThread(ThreadType *thread, ThreadFunction function, void *argument, void *stack, size_t stack_size, s32 priority) {
        return this->CreateThread(thread, function, argument, stack, stack_size, priority, this->impl.GetDefaultCoreNumber());
    }

    void ThreadManager::DestroyThread(ThreadType *thread) {
        {
            std::scoped_lock lk(GetReference(thread->cs_thread));

            if (thread->state == ThreadType::State_Initialized) {
                thread->state = ThreadType::State_DestroyedBeforeStarted;
                this->impl.StartThread(thread);
                GetReference(thread->cv_thread).Signal();
            }
        }

        this->impl.WaitForThreadExit(thread);

        AMS_ASSERT(thread->state == ThreadType::State_Initialized);

        {
            std::scoped_lock lk(GetReference(thread->cs_thread));

            /* NOTE: Here Nintendo would cleanup the alias stack. */

            this->impl.DestroyThreadUnsafe(thread);

            thread->state = ThreadType::State_NotInitialized;

            GetReference(thread->waitlist).~WaitableObjectList();

            thread->name_buffer[0] = '\x00';

            {
                std::scoped_lock tlk(this->cs);
                this->EraseFromAllThreadsListUnsafe(thread);
            }
        }
    }

    void ThreadManager::StartThread(ThreadType *thread) {
        std::scoped_lock lk(GetReference(thread->cs_thread));

        AMS_ASSERT(thread->state == ThreadType::State_Initialized);

        this->impl.StartThread(thread);
        thread->state = ThreadType::State_Started;

        GetReference(thread->cv_thread).Signal();
    }

    void ThreadManager::WaitThread(ThreadType *thread) {
        this->impl.WaitForThreadExit(thread);

        {
            std::scoped_lock lk(GetReference(thread->cs_thread));

            /* Note: Here Nintendo would cleanup the alias stack. */
        }
    }

    bool ThreadManager::TryWaitThread(ThreadType *thread) {
        const bool result = this->impl.TryWaitForThreadExit(thread);

        if (result) {
            std::scoped_lock lk(GetReference(thread->cs_thread));

            /* Note: Here Nintendo would cleanup the alias stack. */
        }

        return result;
    }

    s32 ThreadManager::SuspendThread(ThreadType *thread) {
        std::scoped_lock lk(GetReference(thread->cs_thread));

        auto prev_suspend_count = thread->suspend_count;
        AMS_ASSERT(prev_suspend_count < ThreadSuspendCountMax);
        thread->suspend_count = prev_suspend_count + 1;

        if (prev_suspend_count == 0) {
            this->impl.SuspendThreadUnsafe(thread);
        }
        return prev_suspend_count;
    }

    s32 ThreadManager::ResumeThread(ThreadType *thread) {
        std::scoped_lock lk(GetReference(thread->cs_thread));

        auto prev_suspend_count = thread->suspend_count;
        if (prev_suspend_count > 0) {
            thread->suspend_count = prev_suspend_count - 1;
            if (prev_suspend_count == 1) {
                this->impl.ResumeThreadUnsafe(thread);
            }
        }
        return prev_suspend_count;
    }

    void ThreadManager::CancelThreadSynchronization(ThreadType *thread) {
        std::scoped_lock lk(GetReference(thread->cs_thread));

        this->impl.CancelThreadSynchronizationUnsafe(thread);
    }

    /* TODO void ThreadManager::GetThreadContext(ThreadContextInfo *out_context, const ThreadType *thread); */

    void ThreadManager::SetInitialThreadNameUnsafe(ThreadType *thread) {
        if (thread == std::addressof(this->main_thread)) {
            constexpr const char MainThreadName[] = "MainThread";
            static_assert(sizeof(thread->name_buffer) >= sizeof(MainThreadName));
            static_assert(MainThreadName[sizeof(MainThreadName) - 1] == '\x00');
            std::memcpy(thread->name_buffer, MainThreadName, sizeof(MainThreadName));
        } else {
            constexpr const char ThreadNamePrefix[] = "Thread_0x";
            constexpr size_t ThreadNamePrefixSize = sizeof(ThreadNamePrefix) - 1;
            const u64 func = reinterpret_cast<u64>(thread->function);
            static_assert(ThreadNamePrefixSize + sizeof(func) * 2 + 1 <= sizeof(thread->name_buffer));
            std::snprintf(thread->name_buffer, sizeof(thread->name_buffer), "%s%016lX", ThreadNamePrefix, func);
        }

        thread->name_pointer = thread->name_buffer;
    }

}
