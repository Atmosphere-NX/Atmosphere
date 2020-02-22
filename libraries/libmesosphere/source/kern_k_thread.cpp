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
        this->cond_var                      = nullptr;

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

        /* We don't need a release (probably), and we've spent no time on the cpu. */
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
        MESOSPHERE_UNIMPLEMENTED();
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

    void KThread::DoWorkerTask() {
        MESOSPHERE_UNIMPLEMENTED();
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

            /* Bind outselves to this core. */
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
            this->original_affinity_mask = this->affinity_mask;

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

    void KThread::TrySuspend() {
        MESOSPHERE_ASSERT_THIS();
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());
        MESOSPHERE_ASSERT(this->IsSuspended());

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
        MESOSPHERE_ASSERT(this->IsSuspended());

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
            if (auto *cond_var = thread->GetConditionVariable(); cond_var != nullptr) {
                cond_var->BeforeUpdatePriority(thread);
            }

            /* Change the priority. */
            const s32 old_priority = thread->GetPriority();
            thread->SetPriority(new_priority);

            /* Restore the condition variable, if relevant. */
            if (auto *cond_var = thread->GetConditionVariable(); cond_var != nullptr) {
                cond_var->AfterUpdatePriority(thread);
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

        MESOSPHERE_UNIMPLEMENTED();

        MESOSPHERE_PANIC("KThread::Exit() would return");
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

}
