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
#include "os_thread_manager_impl.os.windows.hpp"
#include "os_thread_manager.hpp"

namespace ams::os::impl {

    namespace {

        constexpr size_t DefaultStackSize = 1_MB;

        class ScopedSaveLastError {
            NON_COPYABLE(ScopedSaveLastError);
            NON_MOVEABLE(ScopedSaveLastError);
            private:
                DWORD m_last_error;
            public:
                ALWAYS_INLINE ScopedSaveLastError() : m_last_error(::GetLastError()) { /* ... */ }
                ALWAYS_INLINE ~ScopedSaveLastError() { ::SetLastError(m_last_error); }
        };

        unsigned __stdcall InvokeThread(void *arg) {
            ThreadType *thread      = static_cast<ThreadType *>(arg);

            /* Invoke the thread. */
            ThreadManager::InvokeThread(thread);

            return 0;
        }

        os::NativeHandle DuplicateThreadHandle(os::NativeHandle thread_handle) {
            /* Get the thread's windows handle. */
            os::NativeHandle windows_handle = os::InvalidNativeHandle;
            AMS_ABORT_UNLESS(::DuplicateHandle(::GetCurrentProcess(), thread_handle, ::GetCurrentProcess(), std::addressof(windows_handle), 0, 0, DUPLICATE_SAME_ACCESS));

            return windows_handle;
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

            /* Set the thread's windows handle. */
            thread->native_handle = DuplicateThreadHandle(::GetCurrentThread());
            thread->ideal_core    = ::GetCurrentProcessorNumber();
            thread->affinity_mask = thread_manager.GetThreadAvailableCoreMask();

            /* Place the object under the thread manager. */
            thread_manager.PlaceThreadObjectUnderThreadManagerSafe(thread);

            return thread;
        }

    }

    ThreadManagerWindowsImpl::ThreadManagerWindowsImpl(ThreadType *main_thread) : m_tls_index(::TlsAlloc()) {
        /* Verify tls index is valid. */
        AMS_ASSERT(m_tls_index != TLS_OUT_OF_INDEXES);

        /* Setup the main thread object. */
        SetupThreadObjectUnsafe(main_thread, nullptr, nullptr, nullptr, nullptr, DefaultStackSize, DefaultThreadPriority);


        /* Setup the main thread's windows handle information. */
        main_thread->native_handle = DuplicateThreadHandle(::GetCurrentThread());
        main_thread->ideal_core    = ::GetCurrentProcessorNumber();
        main_thread->affinity_mask = this->GetThreadAvailableCoreMask();
    }

    Result ThreadManagerWindowsImpl::CreateThread(ThreadType *thread, s32 ideal_core) {
        /* Create the thread. */
        os::NativeHandle thread_handle = reinterpret_cast<os::NativeHandle>(_beginthreadex(nullptr, thread->stack_size, &InvokeThread, thread, CREATE_SUSPENDED, 0));
        AMS_ASSERT(thread_handle != os::InvalidNativeHandle);

        /* Set the thread's windows handle information. */
        thread->native_handle = thread_handle;
        thread->ideal_core    = ideal_core;
        thread->affinity_mask = this->GetThreadAvailableCoreMask();

        R_SUCCEED();
    }

    void ThreadManagerWindowsImpl::DestroyThreadUnsafe(ThreadType *thread) {
        /* Close the thread's handle. */
        const auto ret = ::CloseHandle(thread->native_handle);
        AMS_ASSERT(ret);
        AMS_UNUSED(ret);

        thread->native_handle = os::InvalidNativeHandle;
    }

    void ThreadManagerWindowsImpl::StartThread(const ThreadType *thread) {
        ScopedSaveLastError save_error;

        /* Resume the thread. */
        const auto ret = ::ResumeThread(thread->native_handle);
        AMS_ASSERT(ret == 1);
        AMS_UNUSED(ret);
    }

    void ThreadManagerWindowsImpl::WaitForThreadExit(ThreadType *thread) {
        ::WaitForSingleObject(thread->native_handle, INFINITE);
    }

    bool ThreadManagerWindowsImpl::TryWaitForThreadExit(ThreadType *thread) {
        return ::WaitForSingleObject(thread->native_handle, 0) == 0;
    }

    void ThreadManagerWindowsImpl::YieldThread() {
        ::Sleep(0);
    }

    bool ThreadManagerWindowsImpl::ChangePriority(ThreadType *thread, s32 priority) {
        AMS_UNUSED(thread, priority);
        return true;
    }

    s32 ThreadManagerWindowsImpl::GetCurrentPriority(const ThreadType *thread) const {
        return thread->base_priority;
    }

    ThreadId ThreadManagerWindowsImpl::GetThreadId(const ThreadType *thread) const {
        ScopedSaveLastError save_error;

        const auto thread_id = ::GetThreadId(thread->native_handle);
        AMS_ASSERT(thread_id != 0);

        return thread_id;
    }

    void ThreadManagerWindowsImpl::SuspendThreadUnsafe(ThreadType *thread) {
        ScopedSaveLastError save_error;

        const auto ret = ::SuspendThread(thread->native_handle);
        AMS_ASSERT(ret == 0);
        AMS_UNUSED(ret);
    }

    void ThreadManagerWindowsImpl::ResumeThreadUnsafe(ThreadType *thread) {
        ScopedSaveLastError save_error;

        const auto ret = ::ResumeThread(thread->native_handle);
        AMS_ASSERT(ret == 1);
        AMS_UNUSED(ret);
    }

    /* TODO: void ThreadManagerWindowsImpl::GetThreadContextUnsafe(ThreadContextInfo *out_context, const ThreadType *thread); */

    void ThreadManagerWindowsImpl::NotifyThreadNameChangedImpl(const ThreadType *thread) const {
        /* TODO */
        AMS_UNUSED(thread);
    }

    void ThreadManagerWindowsImpl::SetCurrentThread(ThreadType *thread) const {
        ScopedSaveLastError save_error;

        ::TlsSetValue(m_tls_index, thread);
    }

    ThreadType *ThreadManagerWindowsImpl::GetCurrentThread() const {
        ScopedSaveLastError save_error;

        /* Get the thread from tls index. */
        ThreadType *thread = static_cast<ThreadType *>(static_cast<void *>(::TlsGetValue(m_tls_index)));

        /* If the thread's TLS isn't set, we need to find it (and set tls) or make it. */
        if (thread == nullptr) {
            /* Try to find the thread. */
            thread = GetThreadManager().FindThreadTypeById(::GetCurrentThreadId());
            if (thread == nullptr) {
                /* Create the thread. */
                thread = DynamicAllocateAndRegisterThreadType();
            }

            /* Set the thread's TLS. */
            this->SetCurrentThread(thread);
        }

        return thread;
    }

    s32 ThreadManagerWindowsImpl::GetCurrentCoreNumber() const {
        ScopedSaveLastError save_error;

        return ::GetCurrentProcessorNumber();
    }

    void ThreadManagerWindowsImpl::SetThreadCoreMask(ThreadType *thread, s32 ideal_core, u64 affinity_mask) const {
        ScopedSaveLastError save_error;

        /* If we should use the default, set the actual ideal core. */
        if (ideal_core == IdealCoreUseDefault) {
            affinity_mask = this->GetThreadAvailableCoreMask();
            ideal_core    = util::CountTrailingZeros(affinity_mask);
            affinity_mask = static_cast<decltype(affinity_mask)>(1) << ideal_core;
        }

        /* Lock the thread. */
        std::scoped_lock lk(util::GetReference(thread->cs_thread));

        /* Set the thread's affinity mask. */
        const auto old = ::SetThreadAffinityMask(thread->native_handle, affinity_mask);
        AMS_ABORT_UNLESS(old != 0);

        /* Set the ideal core. */
        if (ideal_core != IdealCoreNoUpdate) {
            thread->ideal_core = ideal_core;
        }

        /* Set the tracked affinity mask. */
        thread->affinity_mask = affinity_mask;
    }

    void ThreadManagerWindowsImpl::GetThreadCoreMask(s32 *out_ideal_core, u64 *out_affinity_mask, const ThreadType *thread) const {
        ScopedSaveLastError save_error;

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

    u64 ThreadManagerWindowsImpl::GetThreadAvailableCoreMask() const {
        ScopedSaveLastError save_error;

        /* Get the process's affinity mask. */
        u64 process_affinity, system_affinity;
        AMS_ABORT_UNLESS(::GetProcessAffinityMask(::GetCurrentProcess(), std::addressof(process_affinity), std::addressof(system_affinity)) != 0);

        return process_affinity;
    }

}
