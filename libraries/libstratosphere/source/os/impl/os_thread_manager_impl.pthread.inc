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
#include "os_thread_manager_impl.pthread.hpp"
#include "os_thread_manager.hpp"

#include <pthread.h>

#if defined(ATMOSPHERE_OS_LINUX)
#include <sched.h>
#elif defined(ATMOSPHERE_OS_MACOS)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach.h>

namespace {

    struct cpu_set_t {
        uint64_t affinity_mask;
    };

    ALWAYS_INLINE void CPU_ZERO(cpu_set_t *cs) { cs->affinity_mask = 0; }
    ALWAYS_INLINE void CPU_SET(int core, cpu_set_t *cs) { cs->affinity_mask |= (UINT64_C(1) << core); }
    ALWAYS_INLINE bool CPU_ISSET(int core, cpu_set_t *cs) { return cs->affinity_mask & (UINT64_C(1) << core); }

    constexpr size_t CPU_SETSIZE = BITSIZEOF(cpu_set_t{}.affinity_mask);

    int sched_getaffinity(pid_t pid, size_t cpu_size, cpu_set_t *cs) {
        /* Ignore the process id/cpu size arguments. */
        static_cast<void>(pid);
        static_cast<void>(cpu_size);

        /* Get the core count. */
        int32_t core_count = 0;
        size_t size = sizeof(core_count);
        if (const auto ret = ::sysctlbyname("machdep.cpu.core_count", std::addressof(core_count), std::addressof(size), 0, 0); ret != 0) {
            return ret;
        }

        /* Set our cpu set structure. */
        cs->affinity_mask = 0;
        for (auto i = 0; i < core_count; ++i) {
            cs->affinity_mask |= (UINT64_C(1) << i);
        }

        return 0;
    }

    int pthread_setaffinity_np(pthread_t thread, size_t cpu_size, cpu_set_t *cs) {
        /* Ignore the cpu size argument. */
        static_cast<void>(cpu_size);

        /* If the thread is allowed to be on more than one core, we'll ignore it. */
        /* TODO: Do this properly? */
        if (const auto pc = std::popcount(cs->affinity_mask); pc == 0 || pc > 1) {
            return 0;
        }

        /* Create policy to bind to the core. */
        thread_affinity_policy_data_t policy = { std::countr_zero(cs->affinity_mask) };

        /* Get the underlying mach thread. */
        thread_port_t mach_thread = pthread_mach_thread_np(thread);

        /* Set the policy. */
        thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY, reinterpret_cast<thread_policy_t>(std::addressof(policy)), 1);

        return 0;
    }

}
#else
#error "Unknown OS for pthread CoreId get"
#endif

namespace ams::os::impl {

    namespace {

        constexpr size_t DefaultStackSize = 1_MB;

        void *InvokeThread(void *arg) {
            ThreadType *thread = static_cast<ThreadType *>(arg);

            /* Invoke the thread. */
            ThreadManager::InvokeThread(thread);

            /* Set exit state. */
            {
                std::scoped_lock lk(util::GetReference(thread->cs_pthread_exit));
                AMS_ASSERT(thread->exited_pthread == false);

                thread->exited_pthread = true;
                util::GetReference(thread->cv_pthread_exit).Broadcast();
            }

            return nullptr;
        }

        os::ThreadType *DynamicAllocateAndRegisterThreadType() {
            /* Get the thread manager. */
            auto &thread_manager = GetThreadManager();

            /* Allocate a thread. */
            auto *thread = thread_manager.AllocateThreadType();
            AMS_ABORT_UNLESS(thread != nullptr);

            /* Setup the thread object. */
            SetupThreadObjectUnsafe(thread, nullptr, nullptr, nullptr, nullptr, 0, DefaultThreadPriority);
            thread->state           = ThreadType::State_Started;
            thread->auto_registered = true;

            /* Set the thread's pthread handle. */
            thread->pthread       = pthread_self();
            thread->ideal_core    = thread_manager.GetCurrentCoreNumber();
            thread->affinity_mask = thread_manager.GetThreadAvailableCoreMask();

            /* Place the object under the thread manager. */
            thread_manager.PlaceThreadObjectUnderThreadManagerSafe(thread);

            return thread;
        }

    }

    ThreadManagerPthreadImpl::ThreadManagerPthreadImpl(ThreadType *main_thread) {
        /* Create tls slot for thread pointer. */
        AMS_ABORT_UNLESS(pthread_key_create(std::addressof(m_tls_key), nullptr) == 0);

        /* Setup the main thread object. */
        SetupThreadObjectUnsafe(main_thread, nullptr, nullptr, nullptr, nullptr, DefaultStackSize, DefaultThreadPriority);

        /* Setup the main thread's pthread information. */
        main_thread->pthread       = pthread_self();
        main_thread->ideal_core    = this->GetCurrentCoreNumber();
        main_thread->affinity_mask = this->GetThreadAvailableCoreMask();
    }

    Result ThreadManagerPthreadImpl::CreateThread(ThreadType *thread, s32 ideal_core) {
        /* Create the assert. */
        /* TODO: Check for failure properly. */
        pthread_t pthread;
        const auto res = pthread_create(std::addressof(pthread), nullptr, &InvokeThread, thread);
        AMS_ASSERT(res == 0);
        AMS_UNUSED(res);

        /* Set the thread's pthread handle information. */
        thread->pthread       = pthread;
        thread->ideal_core    = ideal_core;
        thread->affinity_mask = this->GetThreadAvailableCoreMask();

        R_SUCCEED();
    }

    void ThreadManagerPthreadImpl::DestroyThreadUnsafe(ThreadType *thread) {
        /* The thread must have exited. */
        {
            std::scoped_lock lk(util::GetReference(thread->cs_pthread_exit));
            AMS_ABORT_UNLESS(thread->exited_pthread);
        }

        /* Join the thread. */
        const auto ret = pthread_join(thread->pthread, nullptr);
        AMS_ASSERT(ret == 0);
        AMS_UNUSED(ret);
    }

    void ThreadManagerPthreadImpl::StartThread(const ThreadType *thread) {
        /* Nothing is actually needed here, because pthreads cannot start suspended. */
        /* TODO: Should we add a condvar/mutex for thread start? */
        AMS_UNUSED(thread);
    }

    void ThreadManagerPthreadImpl::WaitForThreadExit(ThreadType *thread) {
        /* Wait for the thread to exit. */
        {
            std::scoped_lock lk(util::GetReference(thread->cs_pthread_exit));
            while (!thread->exited_pthread) {
                util::GetReference(thread->cv_pthread_exit).Wait(util::GetPointer(thread->cs_pthread_exit));
            }
        }
    }

    bool ThreadManagerPthreadImpl::TryWaitForThreadExit(ThreadType *thread) {
        /* Check if the thread has exited. */
        std::scoped_lock lk(util::GetReference(thread->cs_pthread_exit));

        return thread->exited_pthread;
    }

    void ThreadManagerPthreadImpl::YieldThread() {
        /* NOTE: pthread_yield() is deprecated. */
        const auto ret = sched_yield();
        AMS_ASSERT(ret == 0);
        AMS_UNUSED(ret);
    }

    bool ThreadManagerPthreadImpl::ChangePriority(ThreadType *thread, s32 priority) {
        /* TODO: Should we set the thread's niceness value? */
        AMS_UNUSED(thread, priority);
        return true;
    }

    s32 ThreadManagerPthreadImpl::GetCurrentPriority(const ThreadType *thread) const {
        return thread->base_priority;
    }

    ThreadId ThreadManagerPthreadImpl::GetThreadId(const ThreadType *thread) const {
        #if defined(AMS_OS_IMPL_USE_PTHREADID_NP_FOR_THREAD_ID)
        ThreadId tid;
        const auto ret = pthread_threadid_np(thread->pthread, std::addressof(tid));
        AMS_ABORT_UNLESS(ret == 0);

        return tid;
        #else
        return thread->pthread;
        #endif
    }

    void ThreadManagerPthreadImpl::SuspendThreadUnsafe(ThreadType *thread) {
        AMS_UNUSED(thread);
        AMS_ABORT("TODO: Linux SuspendThread Signal/Pause impl?");
    }

    void ThreadManagerPthreadImpl::ResumeThreadUnsafe(ThreadType *thread) {
        AMS_UNUSED(thread);
        AMS_ABORT("TODO: ResumeThread Signal/Pause impl?");
    }

    void ThreadManagerPthreadImpl::NotifyThreadNameChangedImpl(const ThreadType *thread) const {
        /* TODO */
        AMS_UNUSED(thread);
    }

    void ThreadManagerPthreadImpl::SetCurrentThread(ThreadType *thread) const {
        const auto ret = pthread_setspecific(m_tls_key, thread);
        AMS_ASSERT(ret == 0);
        AMS_UNUSED(ret);
    }

    ThreadType *ThreadManagerPthreadImpl::GetCurrentThread() const {
        /* Get the thread from tls index. */
        ThreadType *thread = static_cast<ThreadType *>(pthread_getspecific(m_tls_key));

        /* If the thread's TLS isn't set, we need to find it (and set tls) or make it. */
        if (thread == nullptr) {
            /* Get the current thread id. */
            ThreadId self_tid;
            pthread_t self_thread = pthread_self();
            #if defined(AMS_OS_IMPL_USE_PTHREADID_NP_FOR_THREAD_ID)
            const auto ret = pthread_threadid_np(self_thread, std::addressof(self_tid));
            AMS_ABORT_UNLESS(ret == 0);
            #else
            self_tid = self_thread;
            #endif

            /* Try to find the thread. */
            thread = GetThreadManager().FindThreadTypeById(self_tid);
            if (thread == nullptr) {
                /* Create the thread. */
                thread = DynamicAllocateAndRegisterThreadType();
            }

            /* Set the thread's TLS. */
            this->SetCurrentThread(thread);
        }

        return thread;
    }

    s32 ThreadManagerPthreadImpl::GetCurrentCoreNumber() const {
        #if defined(ATMOSPHERE_OS_LINUX)
        const auto core = sched_getcpu();
        AMS_ABORT_UNLESS(core >= 0);
        return core;
        #elif defined(ATMOSPHERE_OS_MACOS)
        return 0;
        #else
        AMS_ABORT("TODO: Unknown OS GetCurrentCoreNumber() under pthreads");
        #endif
    }

    void ThreadManagerPthreadImpl::SetThreadCoreMask(ThreadType *thread, s32 ideal_core, u64 affinity_mask) const {
        /* If we should use the default, set the actual ideal core. */
        if (ideal_core == IdealCoreUseDefault) {
            affinity_mask = this->GetThreadAvailableCoreMask();
            ideal_core    = util::CountTrailingZeros(affinity_mask);
            affinity_mask = static_cast<decltype(affinity_mask)>(1) << ideal_core;
        }

        /* Lock the thread. */
        std::scoped_lock lk(util::GetReference(thread->cs_thread));

        /* Build the cpu affinity. */
        cpu_set_t cpuset;
        CPU_ZERO(std::addressof(cpuset));
        for (size_t i = 0; i < std::min<size_t>(BITSIZEOF(affinity_mask), CPU_SETSIZE); ++i) {
            if ((static_cast<decltype(affinity_mask)>(1) << i) & affinity_mask) {
                CPU_SET(i, std::addressof(cpuset));
            }
        }

        /* Set the cpu affinity. */
        const auto ret = pthread_setaffinity_np(thread->pthread, sizeof(cpuset), std::addressof(cpuset));
        AMS_ABORT_UNLESS(ret == 0);

        /* Set the ideal core. */
        if (ideal_core != IdealCoreNoUpdate) {
            thread->ideal_core = ideal_core;
        }

        /* Set the tracked affinity mask. */
        thread->affinity_mask = affinity_mask;
    }

    void ThreadManagerPthreadImpl::GetThreadCoreMask(s32 *out_ideal_core, u64 *out_affinity_mask, const ThreadType *thread) const {
        /* Lock the thread. */
        std::scoped_lock lk(util::GetReference(thread->cs_thread));

        /* Set the output. */
        if (out_ideal_core != nullptr) {
            *out_ideal_core = thread->ideal_core;
        }
        if (out_affinity_mask != nullptr) {
            *out_affinity_mask = thread->affinity_mask;
        }
    }

    u64 ThreadManagerPthreadImpl::GetThreadAvailableCoreMask() const {
        #if defined(ATMOSPHERE_OS_LINUX) || defined(ATMOSPHERE_OS_MACOS)
        cpu_set_t cpuset;
        CPU_ZERO(std::addressof(cpuset));

        const auto ret = sched_getaffinity(0, sizeof(cpuset), std::addressof(cpuset));
        AMS_ASSERT(ret == 0);
        AMS_UNUSED(ret);

        u64 mask = 0;
        for (size_t i = 0; i < std::min<size_t>(BITSIZEOF(mask), CPU_SETSIZE); ++i) {
            if (CPU_ISSET(i, std::addressof(cpuset))) {
                mask |= static_cast<decltype(mask)>(1) << i;
            }
        }

        AMS_ASSERT(mask != 0);
        return mask;
        #else
        AMS_ABORT("TODO: Unknown OS GetThreadAvailableCoreMask() under pthreads");
        #endif
    }

}