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

    void SetupThreadObjectUnsafe(ThreadType *thread, void *platform, ThreadFunction function, void *arg, void *stack, size_t stack_size, s32 priority) {
        /* Clear the thread object. */
        std::memset(thread, 0, sizeof(*thread));

        /* Setup objects. */
        util::ConstructAt(thread->cs_thread);
        util::ConstructAt(thread->cv_thread);

        util::ConstructAt(thread->all_threads_node);
        util::ConstructAt(thread->waitlist);

        /* Set member variables. */
        thread->magic            = os::ThreadType::Magic;
        thread->stack_is_aliased = false;
        thread->auto_registered  = false;
        thread->version          = 0;
        thread->function         = function;
        thread->argument         = arg;
        thread->original_stack   = stack;
        thread->stack            = stack;
        thread->stack_size       = stack_size;
        thread->base_priority    = priority;
        thread->suspend_count    = 0;
        thread->initial_fiber    = nullptr;
        thread->current_fiber    = nullptr;
        thread->name_buffer[0]   = '\x00';
        thread->name_pointer     = thread->name_buffer;

        /* Set platform variables. */
        #if defined(AMS_OS_IMPL_USE_PTHREADS)
        util::ConstructAt(thread->cs_pthread_exit);
        util::ConstructAt(thread->cv_pthread_exit);
        thread->exited_pthread = false;

        std::memset(thread->tls_value_array, 0, sizeof(thread->tls_value_array));
        AMS_UNUSED(platform);
        #elif defined(ATMOSPHERE_OS_HORIZON)
        std::memset(std::addressof(thread->sdk_internal_tls), 0, sizeof(thread->sdk_internal_tls));
        thread->thread_impl = (platform != nullptr) ? static_cast<ThreadType::ThreadImpl *>(platform) : std::addressof(thread->thread_impl_storage);
        #else
        std::memset(thread->tls_value_array, 0, sizeof(thread->tls_value_array));
        AMS_UNUSED(platform);
        #endif

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
        m_total_thread_stack_size = 0;
        m_num_created_threads     = 0;

        m_main_thread.state = ThreadType::State_Started;

        this->SetCurrentThread(std::addressof(m_main_thread));
        this->PlaceThreadObjectUnderThreadManagerSafe(std::addressof(m_main_thread));
    }

    void ThreadManager::CleanupThread(ThreadType *thread) {
        /* TODO: TLS Manager->InvokeTlsDestructors(); */

        std::scoped_lock lk(GetReference(thread->cs_thread));

        thread->state = ThreadType::State_Terminated;

        GetReference(thread->cv_thread).Broadcast();
        GetReference(thread->waitlist).WakeupAllMultiWaitThreadsUnsafe();
    }

    void ThreadManager::CleanupThread() {
        return this->CleanupThread(this->GetCurrentThread());
    }

    bool ThreadManager::CreateAliasStackUnsafe(ThreadType *thread) {
        void *alias_stack;
        if (m_impl.MapAliasStack(std::addressof(alias_stack), thread->stack, thread->stack_size)) {
            thread->stack_is_aliased = true;
            thread->stack            = alias_stack;
            return true;
        } else {
            return false;
        }
    }

    void ThreadManager::DeleteAliasStackUnsafe(ThreadType *thread) {
        AMS_ABORT_UNLESS(m_impl.UnmapAliasStack(thread->stack, thread->original_stack, thread->stack_size));

        thread->stack_is_aliased = false;
        thread->stack            = thread->original_stack;
    }

    Result ThreadManager::CreateThread(ThreadType *thread, ThreadFunction function, void *argument, void *stack, size_t stack_size, s32 priority, s32 ideal_core) {
        SetupThreadObjectUnsafe(thread, nullptr, function, argument, stack, stack_size, priority);
        AMS_ABORT_UNLESS(this->CreateAliasStackUnsafe(thread));
        ON_RESULT_FAILURE {
            this->DeleteAliasStackUnsafe(thread);
            thread->state = ThreadType::State_NotInitialized;
        };

        R_TRY(m_impl.CreateThread(thread, ideal_core));

        this->PlaceThreadObjectUnderThreadManagerSafe(thread);

        R_SUCCEED();
    }

    Result ThreadManager::CreateThread(ThreadType *thread, ThreadFunction function, void *argument, void *stack, size_t stack_size, s32 priority) {
        R_RETURN(this->CreateThread(thread, function, argument, stack, stack_size, priority, m_impl.GetDefaultCoreNumber()));
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

        this->DestroyThreadObject(thread);
    }

    void ThreadManager::DestroyThreadObject(ThreadType *thread) {
        {
            std::scoped_lock lk(GetReference(thread->cs_thread));

            {
                std::scoped_lock tlk(m_cs);
                this->EraseFromAllThreadsListUnsafe(thread);
            }

            if (thread->stack_is_aliased) {
                this->DeleteAliasStackUnsafe(thread);
            }

            m_impl.DestroyThreadUnsafe(thread);

            thread->state = ThreadType::State_NotInitialized;

            thread->name_buffer[0] = '\x00';
            thread->magic = 0xCCCC;

            util::DestroyAt(thread->waitlist);
        }
        util::DestroyAt(thread->cs_thread);
        util::DestroyAt(thread->cv_thread);
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

            if (thread->stack_is_aliased) {
                this->DeleteAliasStackUnsafe(thread);
            }
        }
    }

    bool ThreadManager::TryWaitThread(ThreadType *thread) {
        const bool result = m_impl.TryWaitForThreadExit(thread);

        if (result) {
            std::scoped_lock lk(GetReference(thread->cs_thread));

            if (thread->stack_is_aliased) {
                this->DeleteAliasStackUnsafe(thread);
            }
        }

        return result;
    }

    s32 ThreadManager::SuspendThread(ThreadType *thread) {
        std::scoped_lock lk(GetReference(thread->cs_thread));

        const auto prev_suspend_count = thread->suspend_count;
        AMS_ASSERT(prev_suspend_count < ThreadSuspendCountMax);
        thread->suspend_count = prev_suspend_count + 1;

        if (prev_suspend_count == 0) {
            m_impl.SuspendThreadUnsafe(thread);
        }
        return prev_suspend_count;
    }

    s32 ThreadManager::ResumeThread(ThreadType *thread) {
        std::scoped_lock lk(GetReference(thread->cs_thread));

        const auto prev_suspend_count = thread->suspend_count;
        if (prev_suspend_count > 0) {
            thread->suspend_count = prev_suspend_count - 1;
            if (prev_suspend_count == 1) {
                m_impl.ResumeThreadUnsafe(thread);
            }
        }
        return prev_suspend_count;
    }

    #if !defined(ATMOSPHERE_OS_HORIZON)
    void ThreadManager::SetZeroToAllThreadsTlsSafe(int slot) {
        std::scoped_lock lk(m_cs);

        for (auto it = m_all_threads_list.begin(); it != m_all_threads_list.end(); ++it) {
            it->tls_value_array[slot] = 0;
        }
    }
    #endif

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
