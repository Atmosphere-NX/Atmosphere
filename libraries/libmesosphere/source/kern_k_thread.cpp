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

        void CleanupKernelStack(uintptr_t stack_top) {
            const uintptr_t stack_bottom = stack_top - PageSize;

            KPhysicalAddress stack_paddr = Null<KPhysicalAddress>;
            /* TODO: MESOSPHERE_ABORT_UNLESS(Kernel::GetSupervisorPageTable().GetPhysicalAddress(&stack_paddr, stack_bottom)); */
            (void)stack_bottom;

            /* TODO: MESOSPHERE_R_ABORT_UNLESS(Kernel::GetSupervisorPageTable().Unmap(...) */
            (void)stack_paddr;

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
        this->cond_var_tree                 = nullptr;

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
            /* TODO: R_TRY(owner->CreateThreadLocalRegion(&this->tls_address)); */
            /* TODO: this->tls_heap_address = owner->GetThreadLocalRegionAddress(this->tls_address); */
            std::memset(this->tls_heap_address, 0, ams::svc::ThreadLocalRegionSize);
        }

        /* Set parent, if relevant. */
        if (owner != nullptr) {
            this->parent = owner;
            this->parent->Open();
            /* TODO: this->parent->IncrementThreadCount(); */
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
            /* TODO: this->parent->CopySvcPermissionTo(pos.svc_permission); */
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
            /* TODO: this->parent->RegisterThread(this); */
            /* TODO: if (this->parent->IsSuspended()) { this->RequestSuspend(SuspendType_Process); } */
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
            auto page_guard = SCOPE_GUARD { KPageBuffer::Free(page); };
            /* TODO: R_TRY(Kernel::GetSupervisorPageTable().Map); ... */
            (void)(stack_region);
            page_guard.Cancel();
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
        if (owner != nullptr) {
            /* TODO: Release from owner resource limit. */
            (void)(resource_limit_release_hint);
            owner->Close();
        } else {
            /* TODO: Release from system resource limit. */
        }
    }

    void KThread::Finalize() {
        /* TODO */
    }

    bool KThread::IsSignaled() const {
        return this->signaled;
    }

    void KThread::OnTimer() {
        /* TODO */
    }

    void KThread::DoWorkerTask() {
        /* TODO */
    }

    Result KThread::SetPriorityToIdle() {
        MESOSPHERE_ASSERT_THIS();

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
        this->suspend_request_flags |= (1 << (ThreadState_SuspendShift + type));

        /* Try to perform the suspend. */
        this->TrySuspend();
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

        /* TODO */

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
        return std::addressof(this->thread_context);
    }

}
