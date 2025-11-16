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
#include <mesosphere.hpp>

namespace ams::kern {

    namespace {

        constexpr inline s32 TerminatingThreadPriority = ams::svc::SystemThreadPriorityHighest - 1;

        constinit util::Atomic<u64> g_thread_id = 0;

        constexpr ALWAYS_INLINE bool IsKernelAddressKey(KProcessAddress key) {
            const uintptr_t key_uptr = GetInteger(key);
            return KernelVirtualAddressSpaceBase <= key_uptr && key_uptr <= KernelVirtualAddressSpaceLast && (key_uptr & 1) == 0;
        }

        void InitializeKernelStack(uintptr_t stack_top) {
            #if defined(MESOSPHERE_ENABLE_KERNEL_STACK_USAGE)
                const uintptr_t stack_bottom = stack_top - PageSize;
                std::memset(reinterpret_cast<void *>(stack_bottom), 0xCC, PageSize - sizeof(KThread::StackParameters));
            #else
                MESOSPHERE_UNUSED(stack_top);
            #endif
        }

        void CleanupKernelStack(uintptr_t stack_top) {
            const uintptr_t stack_bottom = stack_top - PageSize;

            KPhysicalAddress stack_paddr = Null<KPhysicalAddress>;
            MESOSPHERE_ABORT_UNLESS(Kernel::GetKernelPageTable().GetPhysicalAddress(std::addressof(stack_paddr), stack_bottom));

            MESOSPHERE_R_ABORT_UNLESS(Kernel::GetKernelPageTable().UnmapPages(stack_bottom, 1, KMemoryState_Kernel));

            /* Free the stack page. */
            KPageBuffer::FreeChecked<PageSize>(KPageBuffer::FromPhysicalAddress(stack_paddr));
        }

        class ThreadQueueImplForKThreadSleep final : public KThreadQueueWithoutEndWait { /* ... */ };

        class ThreadQueueImplForKThreadSetProperty final : public KThreadQueue {
            private:
                KThread::WaiterList *m_wait_list;
            public:
                constexpr ThreadQueueImplForKThreadSetProperty(KThread::WaiterList *wl) : m_wait_list(wl) { /* ... */ }

                virtual void CancelWait(KThread *waiting_thread, Result wait_result, bool cancel_timer_task) override {
                    /* Remove the thread from the wait list. */
                    m_wait_list->erase(m_wait_list->iterator_to(*waiting_thread));

                    /* Invoke the base cancel wait handler. */
                    KThreadQueue::CancelWait(waiting_thread, wait_result, cancel_timer_task);
                }
        };

    }

    ALWAYS_INLINE void KThread::SetPinnedSvcPermissions() {
        /* Get our stack parameters. */
        auto &sp = this->GetStackParameters();

        /* Get our parent's svc permissions. */
        MESOSPHERE_ASSERT(m_parent != nullptr);
        const auto &svc_permissions = m_parent->GetSvcPermissions();

        /* Get whether we have access to return from exception. */
        const bool return_from_exception = sp.svc_access_flags[svc::SvcId_ReturnFromException];

        /* Clear all permissions. */
        sp.svc_access_flags.Reset();

        /* Set SynchronizePreemptionState if allowed. */
        if (svc_permissions[svc::SvcId_SynchronizePreemptionState]) {
            sp.svc_access_flags[svc::SvcId_SynchronizePreemptionState] = true;
        }

        /* If we previously had ReturnFromException, potentially grant it and GetInfo. */
        if (return_from_exception) {
            /* Set ReturnFromException (guaranteed allowed, if we're here). */
            sp.svc_access_flags[svc::SvcId_ReturnFromException] = true;

            /* Set GetInfo if allowed. */
            if (svc_permissions[svc::SvcId_GetInfo]) {
                sp.svc_access_flags[svc::SvcId_GetInfo] = true;
            }
        }
    }

    ALWAYS_INLINE void KThread::SetUnpinnedSvcPermissions() {
        /* Get our stack parameters. */
        auto &sp = this->GetStackParameters();

        /* Get our parent's svc permissions. */
        MESOSPHERE_ASSERT(m_parent != nullptr);
        const auto &svc_permissions = m_parent->GetSvcPermissions();

        /* Get whether we have access to return from exception. */
        const bool return_from_exception = sp.svc_access_flags[svc::SvcId_ReturnFromException];

        /* Copy permissions. */
        sp.svc_access_flags = svc_permissions;

        /* Clear specific SVCs based on our state. */
        sp.svc_access_flags[svc::SvcId_SynchronizePreemptionState] = false;

        if (!return_from_exception) {
            sp.svc_access_flags[svc::SvcId_ReturnFromException] = false;
        }
    }

    ALWAYS_INLINE void KThread::SetUsermodeExceptionSvcPermissions() {
        /* Get our stack parameters. */
        auto &sp = this->GetStackParameters();

        /* Get our parent's svc permissions. */
        MESOSPHERE_ASSERT(m_parent != nullptr);
        const auto &svc_permissions = m_parent->GetSvcPermissions();

        /* Set ReturnFromException if allowed. */
        if (svc_permissions[svc::SvcId_ReturnFromException]) {
            sp.svc_access_flags[svc::SvcId_ReturnFromException] = true;
        }

        /* Set GetInfo if allowed. */
        if (svc_permissions[svc::SvcId_GetInfo]) {
            sp.svc_access_flags[svc::SvcId_GetInfo] = true;
        }
    }

    ALWAYS_INLINE void KThread::ClearUsermodeExceptionSvcPermissions() {
        /* Get our stack parameters. */
        auto &sp = this->GetStackParameters();

        /* Clear ReturnFromException. */
        sp.svc_access_flags[svc::SvcId_ReturnFromException] = false;

        /* If pinned, clear GetInfo. */
        if (sp.is_pinned) {
            sp.svc_access_flags[svc::SvcId_GetInfo] = false;
        }
    }

    Result KThread::Initialize(KThreadFunction func, uintptr_t arg, void *kern_stack_top, KProcessAddress user_stack_top, s32 prio, s32 virt_core, KProcess *owner, ThreadType type) {
        /* Assert parameters are valid. */
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(kern_stack_top != nullptr);
        MESOSPHERE_ASSERT((type == ThreadType_Main) || (ams::svc::HighestThreadPriority <= prio && prio <= ams::svc::LowestThreadPriority));
        MESOSPHERE_ASSERT((owner != nullptr) || (type != ThreadType_User));
        MESOSPHERE_ASSERT(0 <= virt_core && virt_core < static_cast<s32>(BITSIZEOF(u64)));

        /* Convert the virtual core to a physical core. */
        const s32 phys_core = cpu::VirtualToPhysicalCoreMap[virt_core];
        MESOSPHERE_ASSERT(0 <= phys_core && phys_core < static_cast<s32>(cpu::NumCores));

        /* First, clear the TLS address. */
        m_tls_address = Null<KProcessAddress>;

        const uintptr_t kern_stack_top_address = reinterpret_cast<uintptr_t>(kern_stack_top);
        MESOSPHERE_UNUSED(kern_stack_top_address);

        /* Next, assert things based on the type. */
        switch (type) {
            case ThreadType_Main:
                {
                    MESOSPHERE_ASSERT(arg == 0);
                }
                [[fallthrough]];
            case ThreadType_HighPriority:
                if (type != ThreadType_Main) {
                    MESOSPHERE_ASSERT(phys_core == GetCurrentCoreId());
                }
                [[fallthrough]];
            case ThreadType_Kernel:
                {
                    MESOSPHERE_ASSERT(user_stack_top == 0);
                    MESOSPHERE_ASSERT(util::IsAligned(kern_stack_top_address, PageSize));
                }
                [[fallthrough]];
            case ThreadType_User:
                {
                    MESOSPHERE_ASSERT(((owner == nullptr) || (owner->GetCoreMask()     | (1ul << virt_core)) == owner->GetCoreMask()));
                    MESOSPHERE_ASSERT(((owner == nullptr) || (owner->GetPriorityMask() | (1ul << prio))      == owner->GetPriorityMask()));
                }
                break;
            default:
                MESOSPHERE_PANIC("KThread::Initialize: Unknown ThreadType %u", static_cast<u32>(type));
                break;
        }

        /* Set the ideal core ID and affinity mask. */
        m_virtual_ideal_core_id  = virt_core;
        m_physical_ideal_core_id = phys_core;
        m_virtual_affinity_mask = (static_cast<u64>(1) << virt_core);
        m_physical_affinity_mask.SetAffinity(phys_core, true);

        /* Set the thread state. */
        m_thread_state                  = (type == ThreadType_Main) ? ThreadState_Runnable : ThreadState_Initialized;

        /* Set TLS address and TLS heap address. */
        /* NOTE: Nintendo wrote TLS address above already, but official code really does write tls address twice. */
        m_tls_address                   = 0;
        m_tls_heap_address              = 0;

        /* Set parent and condvar tree. */
        m_parent                        = nullptr;
        m_condvar_tree                  = nullptr;
        m_condvar_key                   = 0;

        /* Set sync booleans. */
        m_signaled                      = false;
        m_termination_requested         = false;
        m_wait_cancelled                = false;
        m_cancellable                   = false;

        /* Set core ID and wait result. */
        m_core_id                       = phys_core;
        m_wait_result                   = svc::ResultNoSynchronizationObject();

        /* Set the stack top. */
        m_kernel_stack_top              = kern_stack_top;

        /* Set priorities. */
        m_priority                      = prio;
        m_base_priority                 = prio;

        /* Initialize wait queue/sync index. */
        m_synced_index                  = -1;
        m_wait_queue                    = nullptr;

        /* Set suspend flags. */
        m_suspend_request_flags         = 0;
        m_suspend_allowed_flags         = ThreadState_SuspendFlagMask;

        /* We're neither debug attached, nor are we nesting our priority inheritance. */
        m_debug_attached                = false;
        m_priority_inheritance_count    = 0;

        /* We haven't been scheduled, and we have done no light IPC. */
        m_schedule_count                = -1;
        m_last_scheduled_tick           = 0;
        m_light_ipc_data                = nullptr;

        /* We're not waiting for a lock, and we haven't disabled migration. */
        m_waiting_lock_info             = nullptr;
        m_num_core_migration_disables   = 0;

        /* We have no waiters, and no closed objects. */
        m_num_kernel_waiters            = 0;
        m_closed_object                 = nullptr;

        /* Set our current core id. */
        m_current_core_id               = phys_core;

        /* We haven't released our resource limit hint, and we've spent no time on the cpu. */
        m_resource_limit_release_hint   = false;
        m_cpu_time                      = 0;

        /* Setup our kernel stack. */
        if (type != ThreadType_Main) {
            InitializeKernelStack(reinterpret_cast<uintptr_t>(kern_stack_top));
        }

        /* Clear our stack parameters. */
        std::memset(static_cast<void *>(std::addressof(this->GetStackParameters())), 0, sizeof(StackParameters));

        /* Setup the TLS, if needed. */
        if (type == ThreadType_User) {
            R_TRY(owner->CreateThreadLocalRegion(std::addressof(m_tls_address)));
            m_tls_heap_address = owner->GetThreadLocalRegionPointer(m_tls_address);
            std::memset(m_tls_heap_address, 0, ams::svc::ThreadLocalRegionSize);
        }

        /* Set parent, if relevant. */
        if (owner != nullptr) {
            m_parent = owner;
            m_parent->Open();
        }

        /* Initialize thread context. */
        constexpr bool IsDefault64Bit = sizeof(uintptr_t) == sizeof(u64);
        const bool is_64_bit = m_parent ? m_parent->Is64Bit() : IsDefault64Bit;
        const bool is_user = (type == ThreadType_User);
        const bool is_main = (type == ThreadType_Main);
        this->GetContext().Initialize(reinterpret_cast<uintptr_t>(func), reinterpret_cast<uintptr_t>(this->GetStackTop()), GetInteger(user_stack_top), arg, is_user, is_64_bit, is_main);

        /* Setup the stack parameters. */
        StackParameters &sp = this->GetStackParameters();
        if (m_parent != nullptr) {
            this->SetUnpinnedSvcPermissions();
            this->ClearUsermodeExceptionSvcPermissions();
        }
        sp.caller_save_fpu_registers = std::addressof(m_caller_save_fpu_registers);
        sp.cur_thread                = this;
        sp.disable_count             = 1;
        this->SetInExceptionHandler();

        if (m_parent != nullptr && is_64_bit) {
            this->SetFpu64Bit();
        }

        /* Set thread ID. */
        m_thread_id = g_thread_id++;

        /* We initialized! */
        m_initialized = true;

        /* Register ourselves with our parent process. */
        if (m_parent != nullptr) {
            m_parent->RegisterThread(this);
            if (m_parent->IsSuspended()) {
                this->RequestSuspend(SuspendType_Process);
            }
        }

        R_SUCCEED();
    }

    Result KThread::InitializeThread(KThread *thread, KThreadFunction func, uintptr_t arg, KProcessAddress user_stack_top, s32 prio, s32 core, KProcess *owner, ThreadType type) {
        /* Get stack region for the thread. */
        const auto &stack_region = KMemoryLayout::GetKernelStackRegion();
        MESOSPHERE_ABORT_UNLESS(stack_region.GetEndAddress() != 0);

        /* Allocate a page to use as the thread. */
        KPageBuffer *page = KPageBuffer::AllocateChecked<PageSize>();
        R_UNLESS(page != nullptr, svc::ResultOutOfResource());

        /* Map the stack page. */
        KProcessAddress stack_top = Null<KProcessAddress>;
        {
            /* If we fail to map, avoid leaking the page. */
            ON_RESULT_FAILURE { KPageBuffer::Free(page); };

            /* Perform the mapping. */
            KProcessAddress stack_bottom = Null<KProcessAddress>;
            R_TRY(Kernel::GetKernelPageTable().MapPages(std::addressof(stack_bottom), 1, PageSize, page->GetPhysicalAddress(), stack_region.GetAddress(),
                                                        stack_region.GetSize() / PageSize, KMemoryState_Kernel, KMemoryPermission_KernelReadWrite));

            /* Calculate top of the stack. */
            stack_top = stack_bottom + PageSize;
        }

        /* If we fail, cleanup the stack we mapped. */
        ON_RESULT_FAILURE { CleanupKernelStack(GetInteger(stack_top)); };

        /* Initialize the thread. */
        R_RETURN(thread->Initialize(func, arg, GetVoidPointer(stack_top), user_stack_top, prio, core, owner, type));
    }

    void KThread::PostDestroy(uintptr_t arg) {
        KProcess *owner = reinterpret_cast<KProcess *>(arg & ~1ul);
        const bool resource_limit_release_hint = (arg & 1);
        const s64 hint_value = (resource_limit_release_hint ? 0 : 1);
        if (owner != nullptr) {
            owner->ReleaseResource(ams::svc::LimitableResource_ThreadCountMax, 1, hint_value);
            owner->Close();
        } else {
            Kernel::GetSystemResourceLimit().Release(ams::svc::LimitableResource_ThreadCountMax, 1, hint_value);
        }
    }

    void KThread::ResumeThreadsSuspendedForInit() {
        KThread::ListAccessor list_accessor;
        {
            KScopedSchedulerLock sl;

            for (auto &thread : list_accessor) {
                static_cast<KThread &>(thread).Resume(SuspendType_Init);
            }
        }
    }

    void KThread::Finalize() {
        MESOSPHERE_ASSERT_THIS();

        /* If the thread has an owner process, unregister it. */
        if (m_parent != nullptr) {
            m_parent->UnregisterThread(this);
        }

        /* If the thread has a local region, delete it. */
        if (m_tls_address != Null<KProcessAddress>) {
            m_parent->DeleteThreadLocalRegion(m_tls_address);
        }

        /* Release any waiters. */
        {
            MESOSPHERE_ASSERT(m_waiting_lock_info == nullptr);
            KScopedSchedulerLock sl;

            /* Check that we have no kernel waiters. */
            MESOSPHERE_ABORT_UNLESS(m_num_kernel_waiters == 0);

            auto it = m_held_lock_info_list.begin();
            while (it != m_held_lock_info_list.end()) {
                /* Get the lock info. */
                auto * const lock_info = std::addressof(*it);

                /* The lock shouldn't have a kernel waiter. */
                MESOSPHERE_ASSERT(!IsKernelAddressKey(lock_info->GetAddressKey()));

                /* Remove all waiters. */
                while (lock_info->GetWaiterCount() != 0) {
                    /* Get the front waiter. */
                    KThread * const waiter = lock_info->GetHighestPriorityWaiter();

                    /* Remove it from the lock. */
                    if (lock_info->RemoveWaiter(waiter)) {
                        MESOSPHERE_ASSERT(lock_info->GetWaiterCount() == 0);
                    }

                    /* Cancel the thread's wait. */
                    waiter->CancelWait(svc::ResultInvalidState(), true);
                }

                /* Remove the held lock from our list. */
                it = m_held_lock_info_list.erase(it);

                /* Free the lock info. */
                LockWithPriorityInheritanceInfo::Free(lock_info);
            }
        }

        /* Cleanup the kernel stack. */
        if (m_kernel_stack_top != nullptr) {
            CleanupKernelStack(reinterpret_cast<uintptr_t>(m_kernel_stack_top));
        }

        /* Perform inherited finalization. */
        KSynchronizationObject::Finalize();
    }

    bool KThread::IsSignaled() const {
        return m_signaled;
    }

    void KThread::OnTimer() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* If we're waiting, cancel the wait. */
        if (this->GetState() == ThreadState_Waiting) {
            m_wait_queue->CancelWait(this, svc::ResultTimedOut(), false);
        }
    }

    void KThread::StartTermination() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Release user exception and unpin, if relevant. */
        if (m_parent != nullptr) {
            m_parent->ReleaseUserException(this);
            if (m_parent->GetPinnedThread(GetCurrentCoreId()) == this) {
                m_parent->UnpinCurrentThread();
            }
        }

        /* Set state to terminated. */
        this->SetState(KThread::ThreadState_Terminated);

        /* Clear the thread's status as running in parent. */
        if (m_parent != nullptr) {
            m_parent->ClearRunningThread(this);
        }

        /* Call the on thread termination handler. */
        KThreadContext::OnThreadTerminating(this);

        /* Clear previous thread in KScheduler. */
        KScheduler::ClearPreviousThread(this);

        /* Register terminated dpc flag. */
        this->RegisterDpc(DpcFlag_Terminated);
    }

    void KThread::FinishTermination() {
        MESOSPHERE_ASSERT_THIS();

        /* Ensure that the thread is not executing on any core. */
        if (m_parent != nullptr) {
            /* Wait for the thread to not be current on any core. */
            for (size_t i = 0; i < cpu::NumCores; ++i) {
                KThread *core_thread;
                do {
                    core_thread = Kernel::GetScheduler(i).GetSchedulerCurrentThread();
                } while (core_thread == this);
            }

            /* Ensure that all cores are synchronized at this point. */
            cpu::SynchronizeCores(m_parent->GetPhysicalCoreMask());
        }

        /* Acquire the scheduler lock. */
        KScopedSchedulerLock sl;

        /* Signal. */
        m_signaled = true;
        KSynchronizationObject::NotifyAvailable();

        /* Close the thread. */
        this->Close();
    }

    void KThread::DoWorkerTaskImpl() {
        /* Finish the termination that was begun by Exit(). */
        this->FinishTermination();
    }

    void KThread::OnEnterUsermodeException() {
        this->SetUsermodeExceptionSvcPermissions();
        this->SetInUsermodeExceptionHandler();
    }

    void KThread::OnLeaveUsermodeException() {
        this->ClearUsermodeExceptionSvcPermissions();

        /* NOTE: InUsermodeExceptionHandler will be cleared by RestoreContext. */
    }

    void KThread::Pin() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Set ourselves as pinned. */
        this->GetStackParameters().is_pinned = true;

        /* Disable core migration. */
        MESOSPHERE_ASSERT(m_num_core_migration_disables == 0);
        {
            ++m_num_core_migration_disables;

            /* Save our ideal state to restore when we're unpinned. */
            m_original_physical_ideal_core_id = m_physical_ideal_core_id;
            m_original_physical_affinity_mask = m_physical_affinity_mask;

            /* Bind ourselves to this core. */
            const s32 active_core  = this->GetActiveCore();
            const s32 current_core = GetCurrentCoreId();

            this->SetActiveCore(current_core);
            m_physical_ideal_core_id = current_core;
            m_physical_affinity_mask.SetAffinityMask(1ul << current_core);

            if (active_core != current_core || m_physical_affinity_mask.GetAffinityMask() != m_original_physical_affinity_mask.GetAffinityMask()) {
                KScheduler::OnThreadAffinityMaskChanged(this, m_original_physical_affinity_mask, active_core);
            }

            /* Set base priority-on-unpin. */
            const s32 old_base_priority = m_base_priority;
            m_base_priority_on_unpin    = old_base_priority;

            /* Set base priority to higher than any possible process priority. */
            m_base_priority = std::min<s32>(old_base_priority, __builtin_ctzll(this->GetOwnerProcess()->GetPriorityMask()) - 1);
            RestorePriority(this);
        }

        /* Disallow performing thread suspension. */
        {
            /* Update our allow flags. */
            m_suspend_allowed_flags &= ~(1 << (util::ToUnderlying(SuspendType_Thread) + util::ToUnderlying(ThreadState_SuspendShift)));

            /* Update our state. */
            this->UpdateState();
        }

        /* Update our SVC access permissions. */
        this->SetPinnedSvcPermissions();
    }

    void KThread::Unpin() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Set ourselves as unpinned. */
        this->GetStackParameters().is_pinned = false;

        /* Enable core migration. */
        MESOSPHERE_ASSERT(m_num_core_migration_disables == 1);
        {
            --m_num_core_migration_disables;

            /* Restore our original state. */
            const KAffinityMask old_mask = m_physical_affinity_mask;

            m_physical_ideal_core_id = m_original_physical_ideal_core_id;
            m_physical_affinity_mask = m_original_physical_affinity_mask;

            if (m_physical_affinity_mask.GetAffinityMask() != old_mask.GetAffinityMask()) {
                const s32 active_core = this->GetActiveCore();

                if (!m_physical_affinity_mask.GetAffinity(active_core)) {
                    if (m_physical_ideal_core_id >= 0) {
                        this->SetActiveCore(m_physical_ideal_core_id);
                    } else {
                        this->SetActiveCore(BITSIZEOF(unsigned long long) - 1 - __builtin_clzll(m_physical_affinity_mask.GetAffinityMask()));
                    }
                }
                KScheduler::OnThreadAffinityMaskChanged(this, old_mask, active_core);
            }

            m_base_priority = m_base_priority_on_unpin;
            RestorePriority(this);
        }

        /* Allow performing thread suspension (if termination hasn't been requested). */
        if (!this->IsTerminationRequested()) {
            /* Update our allow flags. */
            m_suspend_allowed_flags |= (1 << (util::ToUnderlying(SuspendType_Thread) + util::ToUnderlying(ThreadState_SuspendShift)));

            /* Update our state. */
            this->UpdateState();

            /* Update our SVC access permissions. */
            MESOSPHERE_ASSERT(m_parent != nullptr);
            this->SetUnpinnedSvcPermissions();
        }

        /* Resume any threads that began waiting on us while we were pinned. */
        for (auto it = m_pinned_waiter_list.begin(); it != m_pinned_waiter_list.end(); it = m_pinned_waiter_list.erase(it)) {
            it->EndWait(ResultSuccess());
        }
    }

    void KThread::DisableCoreMigration() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(this == GetCurrentThreadPointer());

        KScopedSchedulerLock sl;
        MESOSPHERE_ASSERT(m_num_core_migration_disables >= 0);
        if ((m_num_core_migration_disables++) == 0) {
            /* Save our ideal state to restore when we can migrate again. */
            m_original_physical_ideal_core_id = m_physical_ideal_core_id;
            m_original_physical_affinity_mask = m_physical_affinity_mask;

            /* Bind ourselves to this core. */
            const s32 active_core = this->GetActiveCore();
            m_physical_ideal_core_id = active_core;
            m_physical_affinity_mask.SetAffinityMask(1ul << active_core);

            if (m_physical_affinity_mask.GetAffinityMask() != m_original_physical_affinity_mask.GetAffinityMask()) {
                KScheduler::OnThreadAffinityMaskChanged(this, m_original_physical_affinity_mask, active_core);
            }
        }
    }

    void KThread::EnableCoreMigration() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(this == GetCurrentThreadPointer());

        KScopedSchedulerLock sl;
        MESOSPHERE_ASSERT(m_num_core_migration_disables > 0);
        if ((--m_num_core_migration_disables) == 0) {
            const KAffinityMask old_mask = m_physical_affinity_mask;

            /* Restore our ideals. */
            m_physical_ideal_core_id = m_original_physical_ideal_core_id;
            m_physical_affinity_mask = m_original_physical_affinity_mask;

            if (m_physical_affinity_mask.GetAffinityMask() != old_mask.GetAffinityMask()) {
                const s32 active_core = this->GetActiveCore();

                if (!m_physical_affinity_mask.GetAffinity(active_core)) {
                    if (m_physical_ideal_core_id >= 0) {
                        this->SetActiveCore(m_physical_ideal_core_id);
                    } else {
                        this->SetActiveCore(BITSIZEOF(unsigned long long) - 1 - __builtin_clzll(m_physical_affinity_mask.GetAffinityMask()));
                    }
                }
                KScheduler::OnThreadAffinityMaskChanged(this, old_mask, active_core);
            }
        }
    }

    Result KThread::GetCoreMask(int32_t *out_ideal_core, u64 *out_affinity_mask) {
        MESOSPHERE_ASSERT_THIS();
        {
            KScopedSchedulerLock sl;

            /* Get the virtual mask. */
            *out_ideal_core    = m_virtual_ideal_core_id;
            *out_affinity_mask = m_virtual_affinity_mask;
        }

        R_SUCCEED();
    }

    void KThread::GetPhysicalCoreMask(int32_t *out_ideal_core, u64 *out_affinity_mask) {
        MESOSPHERE_ASSERT_THIS();
        {
            KScopedSchedulerLock sl;
            MESOSPHERE_ASSERT(m_num_core_migration_disables >= 0);

            /* Select between core mask and original core mask. */
            if (m_num_core_migration_disables == 0) {
                *out_ideal_core    = m_physical_ideal_core_id;
                *out_affinity_mask = m_physical_affinity_mask.GetAffinityMask();
            } else {
                *out_ideal_core    = m_original_physical_ideal_core_id;
                *out_affinity_mask = m_original_physical_affinity_mask.GetAffinityMask();
            }
        }
    }

    Result KThread::SetCoreMask(int32_t core_id, u64 v_affinity_mask) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(m_parent != nullptr);
        MESOSPHERE_ASSERT(v_affinity_mask != 0);
        KScopedLightLock lk(m_activity_pause_lock);

        /* Set the core mask. */
        u64 p_affinity_mask = 0;
        {
            KScopedSchedulerLock sl;
            MESOSPHERE_ASSERT(m_num_core_migration_disables >= 0);

            /* If we're updating, set our ideal virtual core. */
            if (core_id != ams::svc::IdealCoreNoUpdate) {
                m_virtual_ideal_core_id = core_id;
            } else {
                /* Preserve our ideal core id. */
                core_id = m_virtual_ideal_core_id;
                R_UNLESS(((1ul << core_id) & v_affinity_mask) != 0, svc::ResultInvalidCombination());
            }

            /* Set our affinity mask. */
            m_virtual_affinity_mask = v_affinity_mask;

            /* Translate the virtual core to a physical core. */
            if (core_id >= 0) {
                core_id = cpu::VirtualToPhysicalCoreMap[core_id];
            }

            /* Translate the virtual affinity mask to a physical one. */
            p_affinity_mask = cpu::ConvertVirtualCoreMaskToPhysical(v_affinity_mask);

            /* If we haven't disabled migration, perform an affinity change. */
            if (m_num_core_migration_disables == 0) {
                const KAffinityMask old_mask = m_physical_affinity_mask;

                /* Set our new ideals. */
                m_physical_ideal_core_id = core_id;
                m_physical_affinity_mask.SetAffinityMask(p_affinity_mask);

                if (m_physical_affinity_mask.GetAffinityMask() != old_mask.GetAffinityMask()) {
                    const s32 active_core = this->GetActiveCore();

                    if (active_core >= 0 && !m_physical_affinity_mask.GetAffinity(active_core)) {
                        const s32 new_core = m_physical_ideal_core_id >= 0 ? m_physical_ideal_core_id : BITSIZEOF(unsigned long long) - 1 - __builtin_clzll(m_physical_affinity_mask.GetAffinityMask());
                        this->SetActiveCore(new_core);
                    }
                    KScheduler::OnThreadAffinityMaskChanged(this, old_mask, active_core);
                }
            } else {
                /* Otherwise, we edit the original affinity for restoration later. */
                m_original_physical_ideal_core_id = core_id;
                m_original_physical_affinity_mask.SetAffinityMask(p_affinity_mask);
            }
        }

        /* Update the pinned waiter list. */
        ThreadQueueImplForKThreadSetProperty wait_queue(std::addressof(m_pinned_waiter_list));
        {
            bool retry_update;
            do {
                /* Lock the scheduler. */
                KScopedSchedulerLock sl;

                /* Don't do any further management if our termination has been requested. */
                R_SUCCEED_IF(this->IsTerminationRequested());

                /* By default, we won't need to retry. */
                retry_update = false;

                /* Check if the thread is currently running. */
                bool thread_is_current = false;
                s32 thread_core;
                for (thread_core = 0; thread_core < static_cast<s32>(cpu::NumCores); ++thread_core) {
                    if (Kernel::GetScheduler(thread_core).GetSchedulerCurrentThread() == this) {
                        thread_is_current = true;
                        break;
                    }
                }

                /* If the thread is currently running, check whether it's no longer allowed under the new mask. */
                if (thread_is_current && ((1ul << thread_core) & p_affinity_mask) == 0) {
                    /* If the thread is pinned, we want to wait until it's not pinned. */
                    if (this->GetStackParameters().is_pinned) {
                        /* Verify that the current thread isn't terminating. */
                        R_UNLESS(!GetCurrentThread().IsTerminationRequested(), svc::ResultTerminationRequested());

                        /* Wait until the thread isn't pinned any more. */
                        m_pinned_waiter_list.push_back(GetCurrentThread());
                        GetCurrentThread().BeginWait(std::addressof(wait_queue));
                    } else {
                        /* If the thread isn't pinned, release the scheduler lock and retry until it's not current. */
                        retry_update = true;
                    }
                }
            } while (retry_update);
        }

        R_SUCCEED();
    }

    void KThread::SetBasePriority(s32 priority) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(ams::svc::HighestThreadPriority <= priority && priority <= ams::svc::LowestThreadPriority);

        KScopedSchedulerLock sl;

        /* Determine the priority value to use. */
        const s32 target_priority = m_termination_requested.Load() && priority >= TerminatingThreadPriority ? TerminatingThreadPriority : priority;

        /* Change our base priority. */
        if (this->GetStackParameters().is_pinned) {
            m_base_priority_on_unpin = target_priority;
        } else {
            m_base_priority = target_priority;
        }

        /* Perform a priority restoration. */
        RestorePriority(this);
    }

    void KThread::IncreaseBasePriority(s32 priority) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(ams::svc::HighestThreadPriority <= priority && priority <= ams::svc::LowestThreadPriority);
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());
        MESOSPHERE_ASSERT(!this->GetStackParameters().is_pinned);

        /* Set our base priority. */
        if (m_base_priority > priority) {
            m_base_priority = priority;

            /* Perform a priority restoration. */
            RestorePriority(this);
        }
    }

    void KThread::SetPriorityToIdle() {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock sl;

        /* Change both our priorities to the idle thread priority. */
        const s32 old_priority = m_priority;
        m_priority      = IdleThreadPriority;
        m_base_priority = IdleThreadPriority;
        KScheduler::OnThreadPriorityChanged(this, old_priority);
    }

    void KThread::RequestSuspend(SuspendType type) {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock lk;

        /* Note the request in our flags. */
        m_suspend_request_flags |= (1u << (util::ToUnderlying(ThreadState_SuspendShift) + util::ToUnderlying(type)));

        /* Try to perform the suspend. */
        this->TrySuspend();
    }

    void KThread::Resume(SuspendType type) {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock sl;

        /* Clear the request in our flags. */
        m_suspend_request_flags &= ~(1u << (util::ToUnderlying(ThreadState_SuspendShift) + util::ToUnderlying(type)));

        /* Update our state. */
        this->UpdateState();
    }

    void KThread::WaitCancel() {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock sl;

        /* Check if we're waiting and cancellable. */
        if (this->GetState() == ThreadState_Waiting && m_cancellable) {
            m_wait_cancelled = false;
            m_wait_queue->CancelWait(this, svc::ResultCancelled(), true);
        } else {
            /* Otherwise, note that we cancelled a wait. */
            m_wait_cancelled = true;
        }
    }

    void KThread::TrySuspend() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());
        MESOSPHERE_ASSERT(this->IsSuspendRequested());

        /* Ensure that we have no waiters. */
        if (this->GetNumKernelWaiters() > 0) {
            return;
        }
        MESOSPHERE_ABORT_UNLESS(this->GetNumKernelWaiters() == 0);

        /* Perform the suspend. */
        this->UpdateState();
    }

    void KThread::UpdateState() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Set our suspend flags in state. */
        const auto old_state = m_thread_state;
        const auto new_state = static_cast<ThreadState>(this->GetSuspendFlags() | (old_state & ThreadState_Mask));
        m_thread_state = new_state;

        /* Note the state change in scheduler. */
        if (new_state != old_state) {
            KScheduler::OnThreadStateChanged(this, old_state);
        }
    }

    void KThread::Continue() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Clear our suspend flags in state. */
        const auto old_state = m_thread_state;
        m_thread_state = static_cast<ThreadState>(old_state & ThreadState_Mask);

        /* Note the state change in scheduler. */
        KScheduler::OnThreadStateChanged(this, old_state);
    }

    size_t KThread::GetKernelStackUsage() const {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(m_kernel_stack_top != nullptr);

        #if defined(MESOSPHERE_ENABLE_KERNEL_STACK_USAGE)
            const u8 *stack = static_cast<const u8 *>(m_kernel_stack_top) - PageSize;

            size_t i;
            for (i = 0; i < PageSize; ++i) {
                if (stack[i] != 0xCC) {
                    break;
                }
            }

            return PageSize - i;
        #else
            return 0;
        #endif
    }

    Result KThread::SetActivity(ams::svc::ThreadActivity activity) {
        /* Lock ourselves. */
        KScopedLightLock lk(m_activity_pause_lock);

        /* Set the activity. */
        {
            /* Lock the scheduler. */
            KScopedSchedulerLock sl;

            /* Verify our state. */
            const auto cur_state = this->GetState();
            R_UNLESS((cur_state == ThreadState_Waiting || cur_state == ThreadState_Runnable), svc::ResultInvalidState());

            /* Either pause or resume. */
            if (activity == ams::svc::ThreadActivity_Paused) {
                /* Verify that we're not suspended. */
                R_UNLESS(!this->IsSuspendRequested(SuspendType_Thread), svc::ResultInvalidState());

                /* Suspend. */
                this->RequestSuspend(SuspendType_Thread);
            } else {
                MESOSPHERE_ASSERT(activity == ams::svc::ThreadActivity_Runnable);

                /* Verify that we're suspended. */
                R_UNLESS(this->IsSuspendRequested(SuspendType_Thread), svc::ResultInvalidState());

                /* Resume. */
                this->Resume(SuspendType_Thread);
            }
        }

        /* If the thread is now paused, update the pinned waiter list. */
        if (activity == ams::svc::ThreadActivity_Paused) {
            ThreadQueueImplForKThreadSetProperty wait_queue(std::addressof(m_pinned_waiter_list));

            bool thread_is_current;
            do {
                /* Lock the scheduler. */
                KScopedSchedulerLock sl;

                /* Don't do any further management if our termination has been requested. */
                R_SUCCEED_IF(this->IsTerminationRequested());

                /* By default, treat the thread as not current. */
                thread_is_current = false;

                /* Check whether the thread is pinned. */
                if (this->GetStackParameters().is_pinned) {
                    /* Verify that the current thread isn't terminating. */
                    R_UNLESS(!GetCurrentThread().IsTerminationRequested(), svc::ResultTerminationRequested());

                    /* Wait until the thread isn't pinned any more. */
                    m_pinned_waiter_list.push_back(GetCurrentThread());
                    GetCurrentThread().BeginWait(std::addressof(wait_queue));
                } else {
                    /* Check if the thread is currently running. */
                    /* If it is, we'll need to retry. */
                    for (auto i = 0; i < static_cast<s32>(cpu::NumCores); ++i) {
                        if (Kernel::GetScheduler(i).GetSchedulerCurrentThread() == this) {
                            thread_is_current = true;
                            break;
                        }
                    }
                }
            } while (thread_is_current);
        }

        R_SUCCEED();
    }

    Result KThread::GetThreadContext3(ams::svc::ThreadContext *out) {
        /* Lock ourselves. */
        KScopedLightLock lk(m_activity_pause_lock);

        /* Get the context. */
        {
            /* Lock the scheduler. */
            KScopedSchedulerLock sl;

            /* Verify that we're suspended. */
            R_UNLESS(this->IsSuspendRequested(SuspendType_Thread), svc::ResultInvalidState());

            /* If we're not terminating, get the thread's user context. */
            if (!this->IsTerminationRequested()) {
                GetUserContext(out, this);
            }
        }

        R_SUCCEED();
    }

    void KThread::AddHeldLock(LockWithPriorityInheritanceInfo *lock_info) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Set ourselves as the lock's owner. */
        lock_info->SetOwner(this);

        /* Add the lock to our held list. */
        m_held_lock_info_list.push_front(*lock_info);
    }

    KThread::LockWithPriorityInheritanceInfo *KThread::FindHeldLock(KProcessAddress address_key) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Try to find an existing held lock. */
        for (auto &held_lock : m_held_lock_info_list) {
            if (held_lock.GetAddressKey() == address_key) {
                return std::addressof(held_lock);
            }
        }

        return nullptr;
    }


    void KThread::AddWaiterImpl(KThread *thread) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());
        MESOSPHERE_ASSERT(thread->GetConditionVariableTree() == nullptr);

        /* Get the thread's address key. */
        const auto address_key = thread->GetAddressKey();

        /* Keep track of how many kernel waiters we have. */
        if (IsKernelAddressKey(address_key)) {
            MESOSPHERE_ABORT_UNLESS((m_num_kernel_waiters++) >= 0);
            KScheduler::SetSchedulerUpdateNeeded();
        }

        /* Get the relevant lock info. */
        auto *lock_info = this->FindHeldLock(address_key);
        if (lock_info == nullptr) {
            /* Create a new lock for the address key. */
            lock_info = LockWithPriorityInheritanceInfo::Create(address_key);

            /* Add the new lock to our list. */
            this->AddHeldLock(lock_info);
        }

        /* Add the thread as waiter to the lock info. */
        lock_info->AddWaiter(thread);
    }

    void KThread::RemoveWaiterImpl(KThread *thread) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Keep track of how many kernel waiters we have. */
        if (IsKernelAddressKey(thread->GetAddressKey())) {
            MESOSPHERE_ABORT_UNLESS((m_num_kernel_waiters--) > 0);
            KScheduler::SetSchedulerUpdateNeeded();
        }

        /* Get the info for the lock the thread is waiting on. */
        auto *lock_info = thread->GetWaitingLockInfo();
        MESOSPHERE_ASSERT(lock_info->GetOwner() == this);

        /* Remove the waiter. */
        if (lock_info->RemoveWaiter(thread)) {
            m_held_lock_info_list.erase(m_held_lock_info_list.iterator_to(*lock_info));
            LockWithPriorityInheritanceInfo::Free(lock_info);
        }
    }

    void KThread::RestorePriority(KThread *thread) {
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        while (thread != nullptr) {
            /* We want to inherit priority where possible. */
            s32 new_priority = thread->GetBasePriority();
            for (const auto &held_lock : thread->m_held_lock_info_list) {
                new_priority = std::min(new_priority, held_lock.GetHighestPriorityWaiter()->GetPriority());
            }

            /* If the priority we would inherit is not different from ours, don't do anything. */
            if (new_priority == thread->GetPriority()) {
                return;
            }

            /* Get the owner of whatever lock this thread is waiting on. */
            KThread * const lock_owner = thread->GetLockOwner();

            /* If the thread is waiting on some lock, remove it as a waiter to prevent violating red black tree invariants. */
            if (lock_owner != nullptr) {
                lock_owner->RemoveWaiterImpl(thread);
            }

            /* Ensure we don't violate condition variable red black tree invariants. */
            if (auto *cv_tree = thread->GetConditionVariableTree(); cv_tree != nullptr) {
                BeforeUpdatePriority(cv_tree, thread);
            }

            /* Change the priority. */
            const s32 old_priority = thread->GetPriority();
            thread->SetPriority(new_priority);

            /* Restore the condition variable, if relevant. */
            if (auto *cv_tree = thread->GetConditionVariableTree(); cv_tree != nullptr) {
                AfterUpdatePriority(cv_tree, thread);
            }

            /* If we removed the thread from some lock's waiting list, add it back. */
            if (lock_owner != nullptr) {
                lock_owner->AddWaiterImpl(thread);
            }

            /* Update the scheduler. */
            KScheduler::OnThreadPriorityChanged(thread, old_priority);

            /* Continue inheriting priority. */
            thread = lock_owner;
        }
    }

    void KThread::AddWaiter(KThread *thread) {
        MESOSPHERE_ASSERT_THIS();

        this->AddWaiterImpl(thread);

        /* If the thread has a higher priority than us, we should inherit. */
        if (thread->GetPriority() < this->GetPriority()) {
            RestorePriority(this);
        }
    }

    void KThread::RemoveWaiter(KThread *thread) {
        MESOSPHERE_ASSERT_THIS();
        this->RemoveWaiterImpl(thread);

        /* If our priority is the same as the thread's (and we've inherited), we may need to restore to lower priority. */
        if (this->GetPriority() == thread->GetPriority() && this->GetPriority() < this->GetBasePriority()) {
            RestorePriority(this);
        }
    }

    KThread *KThread::RemoveWaiterByKey(bool *out_has_waiters, KProcessAddress key) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Get the relevant lock info. */
        auto *lock_info = this->FindHeldLock(key);
        if (lock_info == nullptr) {
            *out_has_waiters = false;
            return nullptr;
        }

        /* Remove the lock info from our held list. */
        m_held_lock_info_list.erase(m_held_lock_info_list.iterator_to(*lock_info));

        /* Keep track of how many kernel waiters we have. */
        if (IsKernelAddressKey(lock_info->GetAddressKey())) {
            m_num_kernel_waiters -= lock_info->GetWaiterCount();
            MESOSPHERE_ABORT_UNLESS(m_num_kernel_waiters >= 0);
            KScheduler::SetSchedulerUpdateNeeded();
        }

        MESOSPHERE_ASSERT(lock_info->GetWaiterCount() > 0);

        /* Remove the highest priority waiter from the lock to be the next owner. */
        KThread *next_lock_owner = lock_info->GetHighestPriorityWaiter();
        if (lock_info->RemoveWaiter(next_lock_owner)) {
            /* The new owner was the only waiter. */
            *out_has_waiters = false;

            /* Free the lock info, since it has no waiters. */
            LockWithPriorityInheritanceInfo::Free(lock_info);
        } else {
            /* There are additional waiters on the lock. */
            *out_has_waiters = true;

            /* Add the lock to the new owner's held list. */
            next_lock_owner->AddHeldLock(lock_info);

            /* Keep track of any kernel waiters for the new owner. */
            if (IsKernelAddressKey(lock_info->GetAddressKey())) {
                next_lock_owner->m_num_kernel_waiters += lock_info->GetWaiterCount();
                MESOSPHERE_ABORT_UNLESS(next_lock_owner->m_num_kernel_waiters > 0);

                /* NOTE: No need to set scheduler update needed, because we will have already done so when removing earlier. */
            }
        }

        /* If our priority is the same as the next owner's (and we've inherited), we may need to restore to lower priority. */
        if (this->GetPriority() == next_lock_owner->GetPriority() && this->GetPriority() < this->GetBasePriority()) {
            RestorePriority(this);
            /* NOTE: No need to restore priority on the next lock owner, because it was already the highest priority waiter on the lock. */
        }

        /* Return the next lock owner. */
        return next_lock_owner;
    }

    Result KThread::Run() {
        MESOSPHERE_ASSERT_THIS();

        /* If the kernel hasn't finished initializing, then we should suspend. */
        if (Kernel::GetState() != Kernel::State::Initialized) {
            this->RequestSuspend(SuspendType_Init);
        }
        while (true) {
            KScopedSchedulerLock lk;

            /* If either this thread or the current thread are requesting termination, note it. */
            R_UNLESS(!this->IsTerminationRequested(),              svc::ResultTerminationRequested());
            R_UNLESS(!GetCurrentThread().IsTerminationRequested(), svc::ResultTerminationRequested());

            /* Ensure our thread state is correct. */
            R_UNLESS(this->GetState() == ThreadState_Initialized,  svc::ResultInvalidState());

            /* If the current thread has been asked to suspend, suspend it and retry. */
            if (GetCurrentThread().IsSuspended()) {
                GetCurrentThread().UpdateState();
                continue;
            }

            /* If we're not a kernel thread and we've been asked to suspend, suspend ourselves. */
            if (KProcess *parent = this->GetOwnerProcess(); parent != nullptr) {
                if (this->IsSuspended()) {
                    this->UpdateState();
                }
                parent->IncrementRunningThreadCount();
            }

            /* Open a reference, now that we're running. */
            this->Open();

            /* Set our state and finish. */
            this->SetState(KThread::ThreadState_Runnable);
            R_SUCCEED();
        }
    }

    void KThread::Exit() {
        MESOSPHERE_ASSERT_THIS();

        MESOSPHERE_ASSERT(this == GetCurrentThreadPointer());

        /* Call the debug callback. */
        KDebug::OnExitThread(this);

        /* Release the thread resource hint, running thread count from parent. */
        if (m_parent != nullptr) {
            m_parent->ReleaseResource(ams::svc::LimitableResource_ThreadCountMax, 0, 1);
            m_resource_limit_release_hint = true;
            m_parent->DecrementRunningThreadCount();
        }

        /* Destroy any dependent objects. */
        this->DestroyClosedObjects();

        /* Perform termination. */
        {
            KScopedSchedulerLock sl;

            /* Disallow all suspension. */
            m_suspend_allowed_flags = 0;
            this->UpdateState();

            /* Start termination. */
            this->StartTermination();

            /* Register the thread as a work task. */
            KWorkerTaskManager::AddTask(KWorkerTaskManager::WorkerType_ExitThread, this);
        }

        MESOSPHERE_PANIC("KThread::Exit() would return");
    }

    Result KThread::Terminate() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(this != GetCurrentThreadPointer());

        /* Request the thread terminate if it hasn't already. */
        if (const auto new_state = this->RequestTerminate(); new_state != ThreadState_Terminated) {
            /* If the thread isn't terminated, wait for it to terminate. */
            s32 index;
            KSynchronizationObject *objects[] = { this };
            R_TRY(KSynchronizationObject::Wait(std::addressof(index), objects, 1, ams::svc::WaitInfinite));
        }

        R_SUCCEED();
    }

    KThread::ThreadState KThread::RequestTerminate() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(this != GetCurrentThreadPointer());

        KScopedSchedulerLock sl;

        /* Determine if this is the first termination request. */
        const bool first_request = [&]() ALWAYS_INLINE_LAMBDA -> bool {
            /* Perform an atomic compare-and-swap from false to true. */
            bool expected = false;
            return m_termination_requested.CompareExchangeStrong(expected, true);
        }();

        /* If this is the first request, start termination procedure. */
        if (first_request) {
            /* If the thread is in initialized state, just change state to terminated. */
            if (this->GetState() == ThreadState_Initialized) {
                m_thread_state = ThreadState_Terminated;
                return ThreadState_Terminated;
            }

            /* Register the terminating dpc. */
            this->RegisterDpc(DpcFlag_Terminating);

            /* If the thread is pinned, unpin it. */
            if (this->GetStackParameters().is_pinned) {
                this->GetOwnerProcess()->UnpinThread(this);
            }

            /* If the thread is suspended, continue it. */
            if (this->IsSuspended()) {
                m_suspend_allowed_flags = 0;
                this->UpdateState();
            }

            /* Change the thread's priority to be higher than any system thread's. */
            this->IncreaseBasePriority(TerminatingThreadPriority);

            /* If the thread is runnable, send a termination interrupt to other cores. */
            if (this->GetState() == ThreadState_Runnable) {
                if (const u64 core_mask = m_physical_affinity_mask.GetAffinityMask() & ~(1ul << GetCurrentCoreId()); core_mask != 0) {
                    cpu::DataSynchronizationBarrierInnerShareable();
                    Kernel::GetInterruptManager().SendInterProcessorInterrupt(KInterruptName_ThreadTerminate, core_mask);
                }
            }

            /* Wake up the thread. */
            if (this->GetState() == ThreadState_Waiting) {
                m_wait_queue->CancelWait(this, svc::ResultTerminationRequested(), true);
            }
        }

        return this->GetState();
    }

    void KThread::Sleep(s64 timeout) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(!KScheduler::IsSchedulerLockedByCurrentThread());
        MESOSPHERE_ASSERT(this == GetCurrentThreadPointer());
        MESOSPHERE_ASSERT(timeout > 0);

        ThreadQueueImplForKThreadSleep wait_queue;
        KHardwareTimer *timer;
        {
            /* Setup the scheduling lock and sleep. */
            KScopedSchedulerLockAndSleep slp(std::addressof(timer), this, timeout);

            /* Check if the thread should terminate. */
            if (this->IsTerminationRequested()) {
                slp.CancelSleep();
                return;
            }

            /* Wait for the sleep to end. */
            wait_queue.SetHardwareTimer(timer);
            this->BeginWait(std::addressof(wait_queue));
        }
    }

    void KThread::BeginWait(KThreadQueue *queue) {
        /* Set our state as waiting. */
        this->SetState(ThreadState_Waiting);

        /* Set our wait queue. */
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdangling-pointer"
        m_wait_queue = queue;
        #pragma GCC diagnostic pop
    }

    void KThread::NotifyAvailable(KSynchronizationObject *signaled_object, Result wait_result) {
        MESOSPHERE_ASSERT_THIS();

        /* Lock the scheduler. */
        KScopedSchedulerLock sl;

        /* If we're waiting, notify our queue that we're available. */
        if (this->GetState() == ThreadState_Waiting) {
            m_wait_queue->NotifyAvailable(this, signaled_object, wait_result);
        }
    }

    void KThread::EndWait(Result wait_result) {
        MESOSPHERE_ASSERT_THIS();

        /* Lock the scheduler. */
        KScopedSchedulerLock sl;

        /* If we're waiting, notify our queue that we're available. */
        if (this->GetState() == ThreadState_Waiting) {
            m_wait_queue->EndWait(this, wait_result);
        }
    }

    void KThread::CancelWait(Result wait_result, bool cancel_timer_task) {
        MESOSPHERE_ASSERT_THIS();

        /* Lock the scheduler. */
        KScopedSchedulerLock sl;

        /* If we're waiting, notify our queue that we're available. */
        if (this->GetState() == ThreadState_Waiting) {
            m_wait_queue->CancelWait(this, wait_result, cancel_timer_task);
        }
    }

    void KThread::SetState(ThreadState state) {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock sl;

        const ThreadState old_state = m_thread_state;
        m_thread_state = static_cast<ThreadState>((old_state & ~ThreadState_Mask) | (state & ThreadState_Mask));
        if (m_thread_state != old_state) {
            KScheduler::OnThreadStateChanged(this, old_state);
        }
    }

    KThread *KThread::GetThreadFromId(u64 thread_id) {
        /* Lock the list. */
        KThread::ListAccessor accessor;
        const auto end = accessor.end();

        /* Find the object with the right id. */
        if (const auto it = accessor.find_key(thread_id); it != end) {
            /* Try to open the thread. */
            if (KThread *thread = static_cast<KThread *>(std::addressof(*it)); AMS_LIKELY(thread->Open())) {
                MESOSPHERE_ASSERT(thread->GetId() == thread_id);
                return thread;
            }
        }

        /* We failed to find or couldn't open the thread. */
        return nullptr;
    }

    Result KThread::GetThreadList(s32 *out_num_threads, ams::kern::svc::KUserPointer<u64 *> out_thread_ids, s32 max_out_count) {
        /* Lock the list. */
        KThread::ListAccessor accessor;
        const auto end = accessor.end();

        /* Iterate over the list. */
        s32 count = 0;
        for (auto it = accessor.begin(); it != end; ++it) {
            /* If we're within array bounds, write the id. */
            if (count < max_out_count) {
                /* Get the thread id. */
                KThread *thread = static_cast<KThread *>(std::addressof(*it));
                const u64 id = thread->GetId();

                /* Copy the id to userland. */
                R_TRY(out_thread_ids.CopyArrayElementFrom(std::addressof(id), count));
            }

            /* Increment the count. */
            ++count;
        }

        /* We successfully iterated the list. */
        *out_num_threads = count;
        R_SUCCEED();
    }

}
