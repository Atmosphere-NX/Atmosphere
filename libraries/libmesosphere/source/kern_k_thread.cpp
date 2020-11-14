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
#include <mesosphere.hpp>

namespace ams::kern {

    namespace {

        constexpr bool IsKernelAddressKey(KProcessAddress key) {
            const uintptr_t key_uptr = GetInteger(key);
            return KernelVirtualAddressSpaceBase <= key_uptr && key_uptr <= KernelVirtualAddressSpaceLast;
        }

        void CleanupKernelStack(uintptr_t stack_top) {
            const uintptr_t stack_bottom = stack_top - PageSize;

            KPhysicalAddress stack_paddr = Null<KPhysicalAddress>;
            MESOSPHERE_ABORT_UNLESS(Kernel::GetKernelPageTable().GetPhysicalAddress(&stack_paddr, stack_bottom));

            MESOSPHERE_R_ABORT_UNLESS(Kernel::GetKernelPageTable().UnmapPages(stack_bottom, 1, KMemoryState_Kernel));

            /* Free the stack page. */
            KPageBuffer::Free(KPageBuffer::FromPhysicalAddress(stack_paddr));
        }

    }

    Result KThread::Initialize(KThreadFunction func, uintptr_t arg, void *kern_stack_top, KProcessAddress user_stack_top, s32 prio, s32 core, KProcess *owner, ThreadType type) {
        /* Assert parameters are valid. */
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(kern_stack_top != nullptr);
        MESOSPHERE_ASSERT((type == ThreadType_Main) || (ams::svc::HighestThreadPriority <= prio && prio <= ams::svc::LowestThreadPriority));
        MESOSPHERE_ASSERT((owner != nullptr) || (type != ThreadType_User));
        MESOSPHERE_ASSERT(0 <= core && core < static_cast<s32>(cpu::NumCores));

        /* First, clear the TLS address. */
        this->tls_address = Null<KProcessAddress>;

        const uintptr_t kern_stack_top_address = reinterpret_cast<uintptr_t>(kern_stack_top);

        /* Next, assert things based on the type. */
        switch (type) {
            case ThreadType_Main:
                {
                    MESOSPHERE_ASSERT(arg == 0);
                }
                [[fallthrough]];
            case ThreadType_HighPriority:
                {
                    MESOSPHERE_ASSERT(core == GetCurrentCoreId());
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
                    MESOSPHERE_ASSERT(((owner == nullptr) || (owner->GetCoreMask()     | (1ul << core)) == owner->GetCoreMask()));
                    MESOSPHERE_ASSERT(((owner == nullptr) || (owner->GetPriorityMask() | (1ul << prio)) == owner->GetPriorityMask()));
                }
                break;
            default:
                MESOSPHERE_PANIC("KThread::Initialize: Unknown ThreadType %u", static_cast<u32>(type));
                break;
        }

        /* Set the ideal core ID and affinity mask. */
        this->ideal_core_id                 = core;
        this->affinity_mask.SetAffinity(core, true);

        /* Set the thread state. */
        this->thread_state                  = (type == ThreadType_Main) ? ThreadState_Runnable : ThreadState_Initialized;

        /* Set TLS address and TLS heap address. */
        /* NOTE: Nintendo wrote TLS address above already, but official code really does write tls address twice. */
        this->tls_address                   = 0;
        this->tls_heap_address              = 0;

        /* Set parent and condvar tree. */
        this->parent                        = nullptr;
        this->condvar_tree                  = nullptr;

        /* Set sync booleans. */
        this->signaled                      = false;
        this->ipc_cancelled                 = false;
        this->termination_requested         = false;
        this->wait_cancelled                = false;
        this->cancellable                   = false;

        /* Set core ID and wait result. */
        this->core_id                       = this->ideal_core_id;
        this->wait_result                   = svc::ResultNoSynchronizationObject();

        /* Set the stack top. */
        this->kernel_stack_top              = kern_stack_top;

        /* Set priorities. */
        this->priority                      = prio;
        this->base_priority                 = prio;

        /* Set sync object and waiting lock to null. */
        this->synced_object                 = nullptr;
        this->waiting_lock                  = nullptr;

        /* Initialize sleeping queue. */
        this->sleeping_queue_entry.Initialize();
        this->sleeping_queue                = nullptr;

        /* Set suspend flags. */
        this->suspend_request_flags         = 0;
        this->suspend_allowed_flags         = ThreadState_SuspendFlagMask;

        /* We're neither debug attached, nor are we nesting our priority inheritance. */
        this->debug_attached                = false;
        this->priority_inheritance_count    = 0;

        /* We haven't been scheduled, and we have done no light IPC. */
        this->schedule_count                = -1;
        this->last_scheduled_tick           = 0;
        this->light_ipc_data                = nullptr;

        /* We're not waiting for a lock, and we haven't disabled migration. */
        this->lock_owner                    = nullptr;
        this->num_core_migration_disables   = 0;

        /* We have no waiters, but we do have an entrypoint. */
        this->num_kernel_waiters            = 0;
        this->entrypoint                    = reinterpret_cast<uintptr_t>(func);

        /* We haven't released our resource limit hint, and we've spent no time on the cpu. */
        this->resource_limit_release_hint   = 0;
        this->cpu_time                      = 0;

        /* Clear our stack parameters. */
        std::memset(static_cast<void *>(std::addressof(this->GetStackParameters())), 0, sizeof(StackParameters));

        /* Setup the TLS, if needed. */
        if (type == ThreadType_User) {
            R_TRY(owner->CreateThreadLocalRegion(std::addressof(this->tls_address)));
            this->tls_heap_address = owner->GetThreadLocalRegionPointer(this->tls_address);
            std::memset(this->tls_heap_address, 0, ams::svc::ThreadLocalRegionSize);
        }

        /* Set parent, if relevant. */
        if (owner != nullptr) {
            this->parent = owner;
            this->parent->Open();
            this->parent->IncrementThreadCount();
        }

        /* Initialize thread context. */
        constexpr bool IsDefault64Bit = sizeof(uintptr_t) == sizeof(u64);
        const bool is_64_bit = this->parent ? this->parent->Is64Bit() : IsDefault64Bit;
        const bool is_user = (type == ThreadType_User);
        const bool is_main = (type == ThreadType_Main);
        this->thread_context.Initialize(this->entrypoint, reinterpret_cast<uintptr_t>(this->GetStackTop()), GetInteger(user_stack_top), arg, is_user, is_64_bit, is_main);

        /* Setup the stack parameters. */
        StackParameters &sp = this->GetStackParameters();
        if (this->parent != nullptr) {
            this->parent->CopySvcPermissionsTo(sp);
        }
        sp.context = std::addressof(this->thread_context);
        sp.disable_count = 1;
        this->SetInExceptionHandler();

        /* Set thread ID. */
        this->thread_id = s_next_thread_id++;

        /* We initialized! */
        this->initialized = true;

        /* Register ourselves with our parent process. */
        if (this->parent != nullptr) {
            this->parent->RegisterThread(this);
            if (this->parent->IsSuspended()) {
                this->RequestSuspend(SuspendType_Process);
            }
        }

        return ResultSuccess();
    }

    Result KThread::InitializeThread(KThread *thread, KThreadFunction func, uintptr_t arg, KProcessAddress user_stack_top, s32 prio, s32 core, KProcess *owner, ThreadType type) {
        /* Get stack region for the thread. */
        const auto &stack_region = KMemoryLayout::GetKernelStackRegion();

        /* Allocate a page to use as the thread. */
        KPageBuffer *page = KPageBuffer::Allocate();
        R_UNLESS(page != nullptr, svc::ResultOutOfResource());

        /* Map the stack page. */
        KProcessAddress stack_top = Null<KProcessAddress>;
        {
            KProcessAddress stack_bottom = Null<KProcessAddress>;
            auto page_guard = SCOPE_GUARD { KPageBuffer::Free(page); };
            R_TRY(Kernel::GetKernelPageTable().MapPages(std::addressof(stack_bottom), 1, PageSize, page->GetPhysicalAddress(), stack_region.GetAddress(),
                                                        stack_region.GetSize() / PageSize, KMemoryState_Kernel, KMemoryPermission_KernelReadWrite));
            page_guard.Cancel();


            /* Calculate top of the stack. */
            stack_top = stack_bottom + PageSize;
        }

        /* Initialize the thread. */
        auto map_guard = SCOPE_GUARD { CleanupKernelStack(GetInteger(stack_top)); };
        R_TRY(thread->Initialize(func, arg, GetVoidPointer(stack_top), user_stack_top, prio, core, owner, type));
        map_guard.Cancel();

        return ResultSuccess();
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
        if (this->parent != nullptr) {
            this->parent->UnregisterThread(this);
        }

        /* If the thread has a local region, delete it. */
        if (this->tls_address != Null<KProcessAddress>) {
            MESOSPHERE_R_ABORT_UNLESS(this->parent->DeleteThreadLocalRegion(this->tls_address));
        }

        /* Release any waiters. */
        {
            MESOSPHERE_ASSERT(this->lock_owner == nullptr);
            KScopedSchedulerLock sl;

            auto it = this->waiter_list.begin();
            while (it != this->waiter_list.end()) {
                /* The thread shouldn't be a kernel waiter. */
                MESOSPHERE_ASSERT(!IsKernelAddressKey(it->GetAddressKey()));
                it->SetLockOwner(nullptr);
                it->SetSyncedObject(nullptr, svc::ResultInvalidState());
                it->Wakeup();
                it = this->waiter_list.erase(it);
            }
        }

        /* Finalize the thread context. */
        this->thread_context.Finalize();

        /* Cleanup the kernel stack. */
        if (this->kernel_stack_top != nullptr) {
            CleanupKernelStack(reinterpret_cast<uintptr_t>(this->kernel_stack_top));
        }

        /* Decrement the parent process's thread count. */
        if (this->parent != nullptr) {
            this->parent->DecrementThreadCount();
        }

        /* Perform inherited finalization. */
        KAutoObjectWithSlabHeapAndContainer<KThread, KSynchronizationObject>::Finalize();
    }

    bool KThread::IsSignaled() const {
        return this->signaled;
    }

    void KThread::Wakeup() {
        MESOSPHERE_ASSERT_THIS();
        KScopedSchedulerLock sl;

        if (this->GetState() == ThreadState_Waiting) {
            if (this->sleeping_queue != nullptr) {
                this->sleeping_queue->WakeupThread(this);
            } else {
                this->SetState(ThreadState_Runnable);
            }
        }
    }

    void KThread::OnTimer() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        this->Wakeup();
    }

    void KThread::StartTermination() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Release user exception and unpin, if relevant. */
        if (this->parent != nullptr) {
            this->parent->ReleaseUserException(this);
            if (this->parent->GetPinnedThread(GetCurrentCoreId()) == this) {
                this->parent->UnpinCurrentThread();
            }
        }

        /* Set state to terminated. */
        this->SetState(KThread::ThreadState_Terminated);

        /* Clear the thread's status as running in parent. */
        if (this->parent != nullptr) {
            this->parent->ClearRunningThread(this);
        }

        /* Signal. */
        this->signaled = true;
        this->NotifyAvailable();

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
        if (this->parent != nullptr) {
            for (size_t i = 0; i < cpu::NumCores; ++i) {
                KThread *core_thread;
                do {
                    core_thread = Kernel::GetCurrentContext(i).current_thread.load(std::memory_order_acquire);
                } while (core_thread == this);
            }
        }

        /* Close the thread. */
        this->Close();
    }

    void KThread::DoWorkerTask() {
        /* Finish the termination that was begun by Exit(). */
        this->FinishTermination();
    }

    void KThread::Pin() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Set ourselves as pinned. */
        this->GetStackParameters().is_pinned = true;

        /* Disable core migration. */
        MESOSPHERE_ASSERT(this->num_core_migration_disables == 0);
        {
            ++this->num_core_migration_disables;

            /* Save our ideal state to restore when we're unpinned. */
            this->original_ideal_core_id = this->ideal_core_id;
            this->original_affinity_mask = this->affinity_mask;

            /* Bind ourselves to this core. */
            const s32 active_core  = this->GetActiveCore();
            const s32 current_core = GetCurrentCoreId();

            this->SetActiveCore(current_core);
            this->ideal_core_id = current_core;

            this->affinity_mask.SetAffinityMask(1ul << current_core);

            if (active_core != current_core || this->affinity_mask.GetAffinityMask() != this->original_affinity_mask.GetAffinityMask()) {
                KScheduler::OnThreadAffinityMaskChanged(this, this->original_affinity_mask, active_core);
            }
        }

        /* Disallow performing thread suspension. */
        {
            /* Update our allow flags. */
            this->suspend_allowed_flags &= ~(1 << (SuspendType_Thread + ThreadState_SuspendShift));

            /* Update our state. */
            const ThreadState old_state = this->thread_state;
            this->thread_state = static_cast<ThreadState>(this->GetSuspendFlags() | (old_state & ThreadState_Mask));
            if (this->thread_state != old_state) {
                KScheduler::OnThreadStateChanged(this, old_state);
            }
        }

        /* Update our SVC access permissions. */
        MESOSPHERE_ASSERT(this->parent != nullptr);
        this->parent->CopyPinnedSvcPermissionsTo(this->GetStackParameters());
    }

    void KThread::Unpin() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Set ourselves as unpinned. */
        this->GetStackParameters().is_pinned = false;

        /* Enable core migration. */
        MESOSPHERE_ASSERT(this->num_core_migration_disables == 1);
        {
            --this->num_core_migration_disables;

            /* Restore our original state. */
            const KAffinityMask old_mask = this->affinity_mask;

            this->ideal_core_id = this->original_ideal_core_id;
            this->affinity_mask = this->original_affinity_mask;

            if (this->affinity_mask.GetAffinityMask() != old_mask.GetAffinityMask()) {
                const s32 active_core = this->GetActiveCore();

                if (!this->affinity_mask.GetAffinity(active_core)) {
                    if (this->ideal_core_id >= 0) {
                        this->SetActiveCore(this->ideal_core_id);
                    } else {
                        this->SetActiveCore(BITSIZEOF(unsigned long long) - 1 - __builtin_clzll(this->affinity_mask.GetAffinityMask()));
                    }
                }
                KScheduler::OnThreadAffinityMaskChanged(this, old_mask, active_core);
            }
        }

        /* Allow performing thread suspension (if termination hasn't been requested). */
        {
            /* Update our allow flags. */
            if (!this->IsTerminationRequested()) {
                this->suspend_allowed_flags |= (1 << (SuspendType_Thread + ThreadState_SuspendShift));
            }

            /* Update our state. */
            const ThreadState old_state = this->thread_state;
            this->thread_state = static_cast<ThreadState>(this->GetSuspendFlags() | (old_state & ThreadState_Mask));
            if (this->thread_state != old_state) {
                KScheduler::OnThreadStateChanged(this, old_state);
            }
        }

        /* Update our SVC access permissions. */
        MESOSPHERE_ASSERT(this->parent != nullptr);
        this->parent->CopyUnpinnedSvcPermissionsTo(this->GetStackParameters());

        /* Resume any threads that began waiting on us while we were pinned. */
        for (auto it = this->pinned_waiter_list.begin(); it != this->pinned_waiter_list.end(); ++it) {
            if (it->GetState() == ThreadState_Waiting) {
                it->SetState(ThreadState_Runnable);
            }
        }
    }

    void KThread::DisableCoreMigration() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(this == GetCurrentThreadPointer());

        KScopedSchedulerLock sl;
        MESOSPHERE_ASSERT(this->num_core_migration_disables >= 0);
        if ((this->num_core_migration_disables++) == 0) {
            /* Save our ideal state to restore when we can migrate again. */
            this->original_ideal_core_id = this->ideal_core_id;
            this->original_affinity_mask = this->affinity_mask;

            /* Bind ourselves to this core. */
            const s32 active_core = this->GetActiveCore();
            this->ideal_core_id = active_core;
            this->affinity_mask.SetAffinityMask(1ul << active_core);

            if (this->affinity_mask.GetAffinityMask() != this->original_affinity_mask.GetAffinityMask()) {
                KScheduler::OnThreadAffinityMaskChanged(this, this->original_affinity_mask, active_core);
            }
        }
    }

    void KThread::EnableCoreMigration() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(this == GetCurrentThreadPointer());

        KScopedSchedulerLock sl;
        MESOSPHERE_ASSERT(this->num_core_migration_disables > 0);
        if ((--this->num_core_migration_disables) == 0) {
            const KAffinityMask old_mask = this->affinity_mask;

            /* Restore our ideals. */
            this->ideal_core_id = this->original_ideal_core_id;
            this->affinity_mask = this->original_affinity_mask;

            if (this->affinity_mask.GetAffinityMask() != old_mask.GetAffinityMask()) {
                const s32 active_core = this->GetActiveCore();

                if (!this->affinity_mask.GetAffinity(active_core)) {
                    if (this->ideal_core_id >= 0) {
                        this->SetActiveCore(this->ideal_core_id);
                    } else {
                        this->SetActiveCore(BITSIZEOF(unsigned long long) - 1 - __builtin_clzll(this->affinity_mask.GetAffinityMask()));
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
            MESOSPHERE_ASSERT(this->num_core_migration_disables >= 0);

            /* Select between core mask and original core mask. */
            if (this->num_core_migration_disables == 0) {
                *out_ideal_core    = this->ideal_core_id;
                *out_affinity_mask = this->affinity_mask.GetAffinityMask();
            } else {
                *out_ideal_core    = this->original_ideal_core_id;
                *out_affinity_mask = this->original_affinity_mask.GetAffinityMask();
            }
        }

        return ResultSuccess();
    }

    Result KThread::SetCoreMask(int32_t ideal_core, u64 affinity_mask) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(this->parent != nullptr);
        MESOSPHERE_ASSERT(affinity_mask != 0);
        KScopedLightLock lk(this->activity_pause_lock);

        /* Set the core mask. */
        {
            KScopedSchedulerLock sl;
            MESOSPHERE_ASSERT(this->num_core_migration_disables >= 0);

            /* If the core id is no-update magic, preserve the ideal core id. */
            if (ideal_core == ams::svc::IdealCoreNoUpdate) {
                if (this->num_core_migration_disables == 0) {
                    ideal_core = this->ideal_core_id;
                } else {
                    ideal_core = this->original_ideal_core_id;
                }

                R_UNLESS(((1ul << ideal_core) & affinity_mask) != 0, svc::ResultInvalidCombination());
            }

            /* If we haven't disabled migration, perform an affinity change. */
            if (this->num_core_migration_disables == 0) {
                const KAffinityMask old_mask = this->affinity_mask;

                /* Set our new ideals. */
                this->ideal_core_id = ideal_core;
                this->affinity_mask.SetAffinityMask(affinity_mask);

                if (this->affinity_mask.GetAffinityMask() != old_mask.GetAffinityMask()) {
                    const s32 active_core = this->GetActiveCore();

                    if (active_core >= 0) {
                        if (!this->affinity_mask.GetAffinity(active_core)) {
                            this->SetActiveCore(this->ideal_core_id);
                        } else {
                            this->SetActiveCore(BITSIZEOF(unsigned long long) - 1 - __builtin_clzll(this->affinity_mask.GetAffinityMask()));
                        }
                    }
                    KScheduler::OnThreadAffinityMaskChanged(this, old_mask, active_core);
                }
            } else {
                /* Otherwise, we edit the original affinity for restoration later. */
                this->original_ideal_core_id = ideal_core;
                this->original_affinity_mask.SetAffinityMask(affinity_mask);
            }
        }

        /* Update the pinned waiter list. */
        {
            bool retry_update;
            bool thread_is_pinned = false;
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
                    if (Kernel::GetCurrentContext(thread_core).current_thread == this) {
                        thread_is_current = true;
                        break;
                    }
                }

                /* If the thread is currently running, check whether it's no longer allowed under the new mask. */
                if (thread_is_current && ((1ul << thread_core) & affinity_mask) == 0) {
                    /* If the thread is pinned, we want to wait until it's not pinned. */
                    if (this->GetStackParameters().is_pinned) {
                        /* Verify that the current thread isn't terminating. */
                        R_UNLESS(!GetCurrentThread().IsTerminationRequested(), svc::ResultTerminationRequested());

                        /* Note that the thread was pinned. */
                        thread_is_pinned = true;

                        /* Wait until the thread isn't pinned any more. */
                        this->pinned_waiter_list.push_back(GetCurrentThread());
                        GetCurrentThread().SetState(ThreadState_Waiting);
                    } else {
                        /* If the thread isn't pinned, release the scheduler lock and retry until it's not current. */
                        retry_update = true;
                    }
                }
            } while (retry_update);

            /* If the thread was pinned, it no longer is, and we should remove the current thread from our waiter list. */
            if (thread_is_pinned) {
                /* Lock the scheduler. */
                KScopedSchedulerLock sl;

                /* Remove from the list. */
                this->pinned_waiter_list.erase(this->pinned_waiter_list.iterator_to(GetCurrentThread()));
            }
        }

        return ResultSuccess();
    }

    void KThread::SetBasePriority(s32 priority) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(ams::svc::HighestThreadPriority <= priority && priority <= ams::svc::LowestThreadPriority);

        KScopedSchedulerLock sl;

        /* Change our base priority. */
        this->base_priority = priority;

        /* Perform a priority restoration. */
        RestorePriority(this);
    }

    Result KThread::SetPriorityToIdle() {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock sl;

        /* Change both our priorities to the idle thread priority. */
        const s32 old_priority = this->priority;
        this->priority      = IdleThreadPriority;
        this->base_priority = IdleThreadPriority;
        KScheduler::OnThreadPriorityChanged(this, old_priority);

        return ResultSuccess();
    }

    void KThread::RequestSuspend(SuspendType type) {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock lk;

        /* Note the request in our flags. */
        this->suspend_request_flags |= (1u << (ThreadState_SuspendShift + type));

        /* Try to perform the suspend. */
        this->TrySuspend();
    }

    void KThread::Resume(SuspendType type) {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock sl;

        /* Clear the request in our flags. */
        this->suspend_request_flags &= ~(1u << (ThreadState_SuspendShift + type));

        /* Update our state. */
        const ThreadState old_state = this->thread_state;
        this->thread_state = static_cast<ThreadState>(this->GetSuspendFlags() | (old_state & ThreadState_Mask));
        if (this->thread_state != old_state) {
            KScheduler::OnThreadStateChanged(this, old_state);
        }
    }

    void KThread::WaitCancel() {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock sl;

        /* Check if we're waiting and cancellable. */
        if (this->GetState() == ThreadState_Waiting && this->cancellable) {
            if (this->sleeping_queue != nullptr) {
                this->sleeping_queue->WakeupThread(this);
                this->wait_cancelled = true;
            } else {
                this->SetSyncedObject(nullptr, svc::ResultCancelled());
                this->SetState(ThreadState_Runnable);
                this->wait_cancelled = false;
            }
        } else {
            /* Otherwise, note that we cancelled a wait. */
            this->wait_cancelled = true;
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
        this->Suspend();
    }

    void KThread::Suspend() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());
        MESOSPHERE_ASSERT(this->IsSuspendRequested());

        /* Set our suspend flags in state. */
        const auto old_state = this->thread_state;
        this->thread_state = static_cast<ThreadState>(this->GetSuspendFlags() | (old_state & ThreadState_Mask));

        /* Note the state change in scheduler. */
        KScheduler::OnThreadStateChanged(this, old_state);
    }

    void KThread::Continue() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Clear our suspend flags in state. */
        const auto old_state = this->thread_state;
        this->thread_state = static_cast<ThreadState>(old_state & ThreadState_Mask);

        /* Note the state change in scheduler. */
        KScheduler::OnThreadStateChanged(this, old_state);
    }

    Result KThread::SetActivity(ams::svc::ThreadActivity activity) {
        /* Lock ourselves. */
        KScopedLightLock lk(this->activity_pause_lock);

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
            bool thread_is_pinned = false;
            bool thread_is_current;
            do {
                /* Lock the scheduler. */
                KScopedSchedulerLock sl;

                /* Don't do any further management if our termination has been requested. */
                R_SUCCEED_IF(this->IsTerminationRequested());

                /* Check whether the thread is pinned. */
                if (this->GetStackParameters().is_pinned) {
                    /* Verify that the current thread isn't terminating. */
                    R_UNLESS(!GetCurrentThread().IsTerminationRequested(), svc::ResultTerminationRequested());

                    /* Note that the thread was pinned and not current. */
                    thread_is_pinned  = true;
                    thread_is_current = false;

                    /* Wait until the thread isn't pinned any more. */
                    this->pinned_waiter_list.push_back(GetCurrentThread());
                    GetCurrentThread().SetState(ThreadState_Waiting);
                } else {
                    /* Check if the thread is currently running. */
                    /* If it is, we'll need to retry. */
                    thread_is_current = false;

                    for (auto i = 0; i < static_cast<s32>(cpu::NumCores); ++i) {
                        if (Kernel::GetCurrentContext(i).current_thread == this) {
                            thread_is_current = true;
                            break;
                        }
                    }
                }
            } while (thread_is_current);

            /* If the thread was pinned, it no longer is, and we should remove the current thread from our waiter list. */
            if (thread_is_pinned) {
                /* Lock the scheduler. */
                KScopedSchedulerLock sl;

                /* Remove from the list. */
                this->pinned_waiter_list.erase(this->pinned_waiter_list.iterator_to(GetCurrentThread()));
            }
        }

        return ResultSuccess();
    }

    Result KThread::GetThreadContext3(ams::svc::ThreadContext *out) {
        /* Lock ourselves. */
        KScopedLightLock lk(this->activity_pause_lock);

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

        return ResultSuccess();
    }

    void KThread::AddWaiterImpl(KThread *thread) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Find the right spot to insert the waiter. */
        auto it = this->waiter_list.begin();
        while (it != this->waiter_list.end()) {
            if (it->GetPriority() > thread->GetPriority()) {
                break;
            }
            it++;
        }

        /* Keep track of how many kernel waiters we have. */
        if (IsKernelAddressKey(thread->GetAddressKey())) {
            MESOSPHERE_ABORT_UNLESS((this->num_kernel_waiters++) >= 0);
        }

        /* Insert the waiter. */
        this->waiter_list.insert(it, *thread);
        thread->SetLockOwner(this);
    }

    void KThread::RemoveWaiterImpl(KThread *thread) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        /* Keep track of how many kernel waiters we have. */
        if (IsKernelAddressKey(thread->GetAddressKey())) {
            MESOSPHERE_ABORT_UNLESS((this->num_kernel_waiters--) > 0);
        }

        /* Remove the waiter. */
        this->waiter_list.erase(this->waiter_list.iterator_to(*thread));
        thread->SetLockOwner(nullptr);
    }

    void KThread::RestorePriority(KThread *thread) {
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        while (true) {
            /* We want to inherit priority where possible. */
            s32 new_priority = thread->GetBasePriority();
            if (thread->HasWaiters()) {
                new_priority = std::min(new_priority, thread->waiter_list.front().GetPriority());
            }

            /* If the priority we would inherit is not different from ours, don't do anything. */
            if (new_priority == thread->GetPriority()) {
                return;
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

            /* Update the scheduler. */
            KScheduler::OnThreadPriorityChanged(thread, old_priority);

            /* Keep the lock owner up to date. */
            KThread *lock_owner = thread->GetLockOwner();
            if (lock_owner == nullptr) {
                return;
            }

            /* Update the thread in the lock owner's sorted list, and continue inheriting. */
            lock_owner->RemoveWaiterImpl(thread);
            lock_owner->AddWaiterImpl(thread);
            thread = lock_owner;
        }
    }

    void KThread::AddWaiter(KThread *thread) {
        MESOSPHERE_ASSERT_THIS();
        this->AddWaiterImpl(thread);
        RestorePriority(this);
    }

    void KThread::RemoveWaiter(KThread *thread) {
        MESOSPHERE_ASSERT_THIS();
        this->RemoveWaiterImpl(thread);
        RestorePriority(this);
    }

    KThread *KThread::RemoveWaiterByKey(s32 *out_num_waiters, KProcessAddress key) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        s32 num_waiters = 0;
        KThread *next_lock_owner = nullptr;
        auto it = this->waiter_list.begin();
        while (it != this->waiter_list.end()) {
            if (it->GetAddressKey() == key) {
                KThread *thread = std::addressof(*it);

                /* Keep track of how many kernel waiters we have. */
                if (IsKernelAddressKey(thread->GetAddressKey())) {
                    MESOSPHERE_ABORT_UNLESS((this->num_kernel_waiters--) > 0);
                }
                it = this->waiter_list.erase(it);

                /* Update the next lock owner. */
                if (next_lock_owner == nullptr) {
                    next_lock_owner = thread;
                    next_lock_owner->SetLockOwner(nullptr);
                } else {
                    next_lock_owner->AddWaiterImpl(thread);
                }
                num_waiters++;
            } else {
                it++;
            }
        }

        /* Do priority updates, if we have a next owner. */
        if (next_lock_owner) {
            RestorePriority(this);
            RestorePriority(next_lock_owner);
        }

        /* Return output. */
        *out_num_waiters = num_waiters;
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
                GetCurrentThread().Suspend();
                continue;
            }

            /* If we're not a kernel thread and we've been asked to suspend, suspend ourselves. */
            if (this->IsUserThread() && this->IsSuspended()) {
                this->Suspend();
            }

            /* Set our state and finish. */
            this->SetState(KThread::ThreadState_Runnable);
            return ResultSuccess();
        }
    }

    void KThread::Exit() {
        MESOSPHERE_ASSERT_THIS();

        MESOSPHERE_ASSERT(this == GetCurrentThreadPointer());

        /* Call the debug callback. */
        KDebug::OnExitThread(this);

        /* Release the thread resource hint from parent. */
        if (this->parent != nullptr) {
            this->parent->ReleaseResource(ams::svc::LimitableResource_ThreadCountMax, 0, 1);
            this->resource_limit_release_hint = true;
        }

        /* Perform termination. */
        {
            KScopedSchedulerLock sl;

            /* Disallow all suspension. */
            this->suspend_allowed_flags = 0;

            /* Start termination. */
            this->StartTermination();

            /* Register the thread as a work task. */
            KWorkerTaskManager::AddTask(KWorkerTaskManager::WorkerType_Exit, this);
        }

        MESOSPHERE_PANIC("KThread::Exit() would return");
    }

    void KThread::Terminate() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(this != GetCurrentThreadPointer());

        /* Request the thread terminate. */
        if (const auto new_state = this->RequestTerminate(); new_state != ThreadState_Terminated) {
            /* If the thread isn't terminated, wait for it to terminate. */
            s32 index;
            KSynchronizationObject *objects[] = { this };
            Kernel::GetSynchronization().Wait(std::addressof(index), objects, 1, ams::svc::WaitInfinite);
        }
    }

    KThread::ThreadState KThread::RequestTerminate() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(this != GetCurrentThreadPointer());

        KScopedSchedulerLock sl;

        /* Determine if this is the first termination request. */
        const bool first_request = [&] ALWAYS_INLINE_LAMBDA () -> bool {
            /* Perform an atomic compare-and-swap from false to true. */
            bool expected = false;
            return this->termination_requested.compare_exchange_strong(expected, true);
        }();

        /* If this is the first request, start termination procedure. */
        if (first_request) {
            /* If the thread is in initialized state, just change state to terminated. */
            if (this->GetState() == ThreadState_Initialized) {
                this->thread_state = ThreadState_Terminated;
                return ThreadState_Terminated;
            }

            /* Register the terminating dpc. */
            this->RegisterDpc(DpcFlag_Terminating);

            /* If the thread is suspended, continue it. */
            if (this->IsSuspended()) {
                this->suspend_allowed_flags = 0;
                this->Continue();
            }

            /* Change the thread's priority to be higher than any system thread's. */
            if (this->GetBasePriority() >= ams::svc::SystemThreadPriorityHighest) {
                this->SetBasePriority(ams::svc::SystemThreadPriorityHighest - 1);
            }

            /* If the thread is runnable, send a termination interrupt to other cores. */
            if (this->GetState() == ThreadState_Runnable) {
                if (const u64 core_mask = this->affinity_mask.GetAffinityMask() & ~(1ul << GetCurrentCoreId()); core_mask != 0) {
                    cpu::DataSynchronizationBarrier();
                    Kernel::GetInterruptManager().SendInterProcessorInterrupt(KInterruptName_ThreadTerminate, core_mask);
                }
            }

            /* Wake up the thread. */
            this->SetSyncedObject(nullptr, svc::ResultTerminationRequested());
            this->Wakeup();
        }

        return this->GetState();
    }

    Result KThread::Sleep(s64 timeout) {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(!KScheduler::IsSchedulerLockedByCurrentThread());
        MESOSPHERE_ASSERT(this == GetCurrentThreadPointer());
        MESOSPHERE_ASSERT(timeout > 0);

        KHardwareTimer *timer;
        {
            /* Setup the scheduling lock and sleep. */
            KScopedSchedulerLockAndSleep slp(std::addressof(timer), this, timeout);

            /* Check if the thread should terminate. */
            if (this->IsTerminationRequested()) {
                slp.CancelSleep();
                return svc::ResultTerminationRequested();
            }

            /* Mark the thread as waiting. */
            this->SetState(KThread::ThreadState_Waiting);
        }

        /* The lock/sleep is done. */

        /* Cancel the timer. */
        timer->CancelTask(this);

        return ResultSuccess();
    }

    void KThread::SetState(ThreadState state) {
        MESOSPHERE_ASSERT_THIS();

        KScopedSchedulerLock sl;

        const ThreadState old_state = this->thread_state;
        this->thread_state = static_cast<ThreadState>((old_state & ~ThreadState_Mask) | (state & ThreadState_Mask));
        if (this->thread_state != old_state) {
            KScheduler::OnThreadStateChanged(this, old_state);
        }
    }

    KThreadContext *KThread::GetContextForSchedulerLoop() {
        return std::addressof(this->GetContext());
    }

    KThread *KThread::GetThreadFromId(u64 thread_id) {
        /* Lock the list. */
        KThread::ListAccessor accessor;
        const auto end = accessor.end();

        /* Define helper object to find the thread. */
        class IdObjectHelper : public KAutoObjectWithListContainer::ListType::value_type {
            private:
                u64 id;
            public:
                constexpr explicit IdObjectHelper(u64 id) : id(id) { /* ... */ }
                virtual u64 GetId() const override { return this->id; }
        };

        /* Find the object with the right id. */
        const auto it = accessor.find(IdObjectHelper(thread_id));

        /* Check to make sure we found the thread. */
        if (it == end) {
            return nullptr;
        }

        /* Get the thread. */
        KThread *thread = static_cast<KThread *>(std::addressof(*it));

        /* Open the thread. */
        if (AMS_LIKELY(thread->Open())) {
            MESOSPHERE_ASSERT(thread->GetId() == thread_id);
            return thread;
        }

        /* We failed to find the thread. */
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
        return ResultSuccess();
    }

}
