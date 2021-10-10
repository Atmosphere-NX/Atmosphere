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
#include "os_thread_manager.hpp"
#include "os_multiple_wait_impl.hpp"
#include "os_multiple_wait_holder_base.hpp"
#include "os_multiple_wait_holder_impl.hpp"
#include "os_multiple_wait_object_list.hpp"
#include "os_utility.hpp"

namespace ams::os::impl {

    void SetupThreadObjectUnsafe(ThreadType *thread, ThreadImpl *thread_impl, ThreadFunction function, void *arg, void *stack, size_t stack_size, s32 priority) {
        /* Setup objects. */
        util::ConstructAt(thread->cs_thread);
        util::ConstructAt(thread->cv_thread);

        util::ConstructAt(thread->all_threads_node);
        util::ConstructAt(thread->waitlist);

        /* Set member variables. */
        thread->magic          = os::ThreadType::Magic;
        thread->version        = 0;
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
            GetReference(thread->cs_thread).Lock();
            while (thread->state == ThreadType::State_Initialized) {
                GetReference(thread->cv_thread).Wait(GetPointer(thread->cs_thread));
            }
            const auto new_state = thread->state;
            GetReference(thread->cs_thread).Unlock();

            if (new_state == ThreadType::State_Started) {
                thread->function(thread->argument);
            }
        }

        manager.CleanupThread();
    }

    ThreadManager::ThreadManager() : m_impl(std::addressof(m_main_thread)), m_total_thread_stack_size(0), m_num_created_threads(0) {
        m_main_thread.state = ThreadType::State_Started;

        this->SetCurrentThread(std::addressof(m_main_thread));

        this->PlaceThreadObjectUnderThreadManagerSafe(std::addressof(m_main_thread));
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
        R_TRY(m_impl.CreateThread(thread, ideal_core));
        guard.Cancel();

        this->PlaceThreadObjectUnderThreadManagerSafe(thread);

        return ResultSuccess();
    }

    Result ThreadManager::CreateThread(ThreadType *thread, ThreadFunction function, void *argument, void *stack, size_t stack_size, s32 priority) {
        return this->CreateThread(thread, function, argument, stack, stack_size, priority, m_impl.GetDefaultCoreNumber());
    }

    void ThreadManager::DestroyThread(ThreadType *thread) {
        {
            std::scoped_lock lk(GetReference(thread->cs_thread));

            if (thread->state == ThreadType::State_Initialized) {
                thread->state = ThreadType::State_DestroyedBeforeStarted;
                m_impl.StartThread(thread);
                GetReference(thread->cv_thread).Signal();
            }
        }

        m_impl.WaitForThreadExit(thread);

        AMS_ASSERT(thread->state == ThreadType::State_Initialized);

        {
            std::scoped_lock lk(GetReference(thread->cs_thread));

            /* NOTE: Here Nintendo would cleanup the alias stack. */

            m_impl.DestroyThreadUnsafe(thread);

            thread->state = ThreadType::State_NotInitialized;

            util::DestroyAt(thread->waitlist);

            thread->name_buffer[0] = '\x00';
            thread->magic = 0xCCCC;

            {
                std::scoped_lock tlk(m_cs);
                this->EraseFromAllThreadsListUnsafe(thread);
            }
        }
    }

    void ThreadManager::StartThread(ThreadType *thread) {
        std::scoped_lock lk(GetReference(thread->cs_thread));

        AMS_ASSERT(thread->state == ThreadType::State_Initialized);

        m_impl.StartThread(thread);
        thread->state = ThreadType::State_Started;

        GetReference(thread->cv_thread).Signal();
    }

    void ThreadManager::WaitThread(ThreadType *thread) {
        m_impl.WaitForThreadExit(thread);

        {
            std::scoped_lock lk(GetReference(thread->cs_thread));

            /* Note: Here Nintendo would cleanup the alias stack. */
        }
    }

    bool ThreadManager::TryWaitThread(ThreadType *thread) {
        const bool result = m_impl.TryWaitForThreadExit(thread);

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
            m_impl.SuspendThreadUnsafe(thread);
        }
        return prev_suspend_count;
    }

    s32 ThreadManager::ResumeThread(ThreadType *thread) {
        std::scoped_lock lk(GetReference(thread->cs_thread));

        auto prev_suspend_count = thread->suspend_count;
        if (prev_suspend_count > 0) {
            thread->suspend_count = prev_suspend_count - 1;
            if (prev_suspend_count == 1) {
                m_impl.ResumeThreadUnsafe(thread);
            }
        }
        return prev_suspend_count;
    }

    void ThreadManager::CancelThreadSynchronization(ThreadType *thread) {
        std::scoped_lock lk(GetReference(thread->cs_thread));

        m_impl.CancelThreadSynchronizationUnsafe(thread);
    }

    /* TODO void ThreadManager::GetThreadContext(ThreadContextInfo *out_context, const ThreadType *thread); */

    namespace {

        constexpr inline const char MainThreadName[] = "MainThread";
        constexpr inline const char ThreadNamePrefix[] = "Thread_0x";

    }

    void ThreadManager::SetInitialThreadNameUnsafe(ThreadType *thread) {
        if (thread == std::addressof(m_main_thread)) {
            static_assert(sizeof(thread->name_buffer) >= sizeof(MainThreadName));
            static_assert(MainThreadName[sizeof(MainThreadName) - 1] == '\x00');
            std::memcpy(thread->name_buffer, MainThreadName, sizeof(MainThreadName));
        } else {
            constexpr size_t ThreadNamePrefixSize = sizeof(ThreadNamePrefix) - 1;
            const u64 func = reinterpret_cast<u64>(thread->function);
            static_assert(ThreadNamePrefixSize + sizeof(func) * 2 + 1 <= sizeof(thread->name_buffer));
            std::memcpy(thread->name_buffer, ThreadNamePrefix, ThreadNamePrefixSize);
            os::impl::ExpandUnsignedValueToAscii(thread->name_buffer + ThreadNamePrefixSize, func);
        }

        thread->name_pointer = thread->name_buffer;
    }

}
