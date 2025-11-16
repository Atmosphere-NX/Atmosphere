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

        ALWAYS_INLINE KDebugBase *GetDebugObject(KProcess *process) {
            return static_cast<KDebugBase *>(process->GetDebugObject());
        }

    }

    void KDebugBase::Initialize() {
        /* Clear the continue flags. */
        m_continue_flags      = 0;
        m_is_force_debug_prod = GetCurrentProcess().CanForceDebugProd();
    }

    bool KDebugBase::Is64Bit() const {
        MESOSPHERE_ASSERT(m_lock.IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(m_is_attached);

        KProcess * const process = this->GetProcessUnsafe();
        MESOSPHERE_ASSERT(process != nullptr);
        return process->Is64Bit();
    }


    Result KDebugBase::QueryMemoryInfo(ams::svc::MemoryInfo *out_memory_info, ams::svc::PageInfo *out_page_info, KProcessAddress address) {
        /* Check that we're attached. */
        R_UNLESS(this->IsAttached(), svc::ResultProcessTerminated());

        /* Open a reference to our process. */
        R_UNLESS(this->OpenProcess(), svc::ResultProcessTerminated());

        /* Close our reference to our process when we're done. */
        ON_SCOPE_EXIT { this->CloseProcess(); };

        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Check that we're still attached now that we're locked. */
        R_UNLESS(this->IsAttached(), svc::ResultProcessTerminated());

        /* Get the process pointer. */
        KProcess * const process = this->GetProcessUnsafe();

        /* Check that the process isn't terminated. */
        R_UNLESS(!process->IsTerminated(), svc::ResultProcessTerminated());

        /* Query the mapping's info. */
        KMemoryInfo info;
        R_TRY(process->GetPageTable().QueryInfo(std::addressof(info), out_page_info, address));

        /* Write output. */
        *out_memory_info = info.GetSvcMemoryInfo();

        R_SUCCEED();
    }

    Result KDebugBase::ReadMemory(KProcessAddress buffer, KProcessAddress address, size_t size) {
        /* Check that we're attached. */
        R_UNLESS(this->IsAttached(), svc::ResultProcessTerminated());

        /* Open a reference to our process. */
        R_UNLESS(this->OpenProcess(), svc::ResultProcessTerminated());

        /* Close our reference to our process when we're done. */
        ON_SCOPE_EXIT { this->CloseProcess(); };

        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Check that we're still attached now that we're locked. */
        R_UNLESS(this->IsAttached(), svc::ResultProcessTerminated());

        /* Get the process pointer. */
        KProcess * const process = this->GetProcessUnsafe();

        /* Check that the process isn't terminated. */
        R_UNLESS(!process->IsTerminated(), svc::ResultProcessTerminated());

        /* Get the page tables. */
        KProcessPageTable &debugger_pt = GetCurrentProcess().GetPageTable();
        KProcessPageTable &target_pt   = process->GetPageTable();

        /* Verify that the regions are in range. */
        R_UNLESS(target_pt.Contains(address, size),  svc::ResultInvalidCurrentMemory());
        R_UNLESS(debugger_pt.Contains(buffer, size), svc::ResultInvalidCurrentMemory());

        /* Iterate over the target process's memory blocks. */
        KProcessAddress cur_address = address;
        size_t remaining = size;
        while (remaining > 0) {
            /* Get the current memory info. */
            KMemoryInfo info;
            ams::svc::PageInfo pi;
            R_TRY(target_pt.QueryInfo(std::addressof(info), std::addressof(pi), cur_address));

            /* Check that the memory is accessible. */
            R_UNLESS(info.GetState() != static_cast<KMemoryState>(ams::svc::MemoryState_Inaccessible), svc::ResultInvalidAddress());

            /* Get the current size. */
            const size_t cur_size = std::min(remaining, info.GetEndAddress() - GetInteger(cur_address));

            /* Read the memory. */
            if (info.GetSvcState() != ams::svc::MemoryState_Io) {
                /* The memory is normal memory. */
                R_TRY(target_pt.ReadDebugMemory(GetVoidPointer(buffer), cur_address, cur_size, this->IsForceDebugProd()));
            } else {
                /* Only allow IO memory to be read if not force debug prod. */
                R_UNLESS(!this->IsForceDebugProd(), svc::ResultInvalidCurrentMemory());

                /* The memory is IO memory. */
                R_TRY(target_pt.ReadDebugIoMemory(GetVoidPointer(buffer), cur_address, cur_size, info.GetState()));
            }

            /* Advance. */
            buffer      += cur_size;
            cur_address += cur_size;
            remaining   -= cur_size;
        }

        R_SUCCEED();
    }

    Result KDebugBase::WriteMemory(KProcessAddress buffer, KProcessAddress address, size_t size) {
        /* Check that we're attached. */
        R_UNLESS(this->IsAttached(), svc::ResultProcessTerminated());

        /* Open a reference to our process. */
        R_UNLESS(this->OpenProcess(), svc::ResultProcessTerminated());

        /* Close our reference to our process when we're done. */
        ON_SCOPE_EXIT { this->CloseProcess(); };

        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Check that we're still attached now that we're locked. */
        R_UNLESS(this->IsAttached(), svc::ResultProcessTerminated());

        /* Get the process pointer. */
        KProcess * const process = this->GetProcessUnsafe();

        /* Check that the process isn't terminated. */
        R_UNLESS(!process->IsTerminated(), svc::ResultProcessTerminated());

        /* Get the page tables. */
        KProcessPageTable &debugger_pt = GetCurrentProcess().GetPageTable();
        KProcessPageTable &target_pt   = process->GetPageTable();

        /* Verify that the regions are in range. */
        R_UNLESS(target_pt.Contains(address, size),  svc::ResultInvalidCurrentMemory());
        R_UNLESS(debugger_pt.Contains(buffer, size), svc::ResultInvalidCurrentMemory());

        /* Iterate over the target process's memory blocks. */
        KProcessAddress cur_address = address;
        size_t remaining = size;
        while (remaining > 0) {
            /* Get the current memory info. */
            KMemoryInfo info;
            ams::svc::PageInfo pi;
            R_TRY(target_pt.QueryInfo(std::addressof(info), std::addressof(pi), cur_address));

            /* Check that the memory is accessible. */
            R_UNLESS(info.GetState() != static_cast<KMemoryState>(ams::svc::MemoryState_Inaccessible), svc::ResultInvalidAddress());

            /* Get the current size. */
            const size_t cur_size = std::min(remaining, info.GetEndAddress() - GetInteger(cur_address));

            /* Read the memory. */
            if (info.GetSvcState() != ams::svc::MemoryState_Io) {
                /* The memory is normal memory. */
                R_TRY(target_pt.WriteDebugMemory(cur_address, GetVoidPointer(buffer), cur_size));
            } else {
                /* The memory is IO memory. */
                R_TRY(target_pt.WriteDebugIoMemory(cur_address, GetVoidPointer(buffer), cur_size, info.GetState()));
            }

            /* Advance. */
            buffer      += cur_size;
            cur_address += cur_size;
            remaining   -= cur_size;
        }

        R_SUCCEED();
    }

    Result KDebugBase::GetRunningThreadInfo(ams::svc::LastThreadContext *out_context, u64 *out_thread_id) {
        /* Check that we're attached. */
        R_UNLESS(this->IsAttached(), svc::ResultProcessTerminated());

        /* Open a reference to our process. */
        R_UNLESS(this->OpenProcess(), svc::ResultProcessTerminated());

        /* Close our reference to our process when we're done. */
        ON_SCOPE_EXIT { this->CloseProcess(); };

        /* Get the process pointer. */
        KProcess * const process = this->GetProcessUnsafe();

        /* Get the thread info. */
        {
            KScopedSchedulerLock sl;

            /* Get the running thread. */
            const s32 core_id = GetCurrentCoreId();
            KThread *thread = process->GetRunningThread(core_id);

            /* We want to check that the thread is actually running. */
            /* If it is, then the scheduler will have just switched from the thread to the current thread. */
            /* This implies exactly one switch will have taken place, and the current thread will be on the current core. */
            const auto &scheduler = Kernel::GetScheduler(core_id);
            if (!(thread != nullptr && thread->GetActiveCore() == core_id && process->GetRunningThreadSwitchCount(core_id) + 1 == scheduler.GetSwitchCount())) {
                /* The most recent thread switch was from a thread other than the expected one to the current one. */
                /* We want to use the appropriate result to inform userland about what thread we switched from. */
                if (scheduler.GetIdleCount() + 1 == scheduler.GetSwitchCount()) {
                    /* We switched from the idle thread. */
                    R_THROW(svc::ResultNoThread());
                } else {
                    /* We switched from some other unknown thread. */
                    R_THROW(svc::ResultUnknownThread());
                }
            }

            /* Get the thread's exception context. */
            GetExceptionContext(thread)->GetSvcThreadContext(out_context);

            /* Get the thread's id. */
            *out_thread_id = thread->GetId();
        }

        R_SUCCEED();
    }

    Result KDebugBase::Attach(KProcess *target) {
        /* Check that the process isn't null. */
        MESOSPHERE_ASSERT(target != nullptr);

        /* Clear ourselves as unattached. */
        m_is_attached = false;

        /* Attach to the process. */
        {
            /* Lock both ourselves, the target process, and the scheduler. */
            KScopedLightLock state_lk(target->GetStateLock());
            KScopedLightLock list_lk(target->GetListLock());
            KScopedLightLock this_lk(m_lock);
            KScopedSchedulerLock sl;

            /* Check that the process isn't already being debugged. */
            R_UNLESS(!target->IsAttachedToDebugger(), svc::ResultBusy());

            {
                /* Ensure the process is in a state that allows for debugging. */
                const KProcess::State state = target->GetState();
                switch (state) {
                    case KProcess::State_Created:
                    case KProcess::State_Running:
                        /* Created and running processes can only be debugged if the debugger is not ForceDebugProd. */
                        R_UNLESS(!this->IsForceDebugProd(), svc::ResultInvalidState());
                        break;
                    case KProcess::State_Crashed:
                        break;
                    case KProcess::State_CreatedAttached:
                    case KProcess::State_RunningAttached:
                    case KProcess::State_DebugBreak:
                        R_THROW(svc::ResultBusy());
                    case KProcess::State_Terminating:
                    case KProcess::State_Terminated:
                        R_THROW(svc::ResultProcessTerminated());
                    MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
                }

                /* Attach to the target. */
                m_process_holder.Attach(target);
                m_is_attached = true;

                /* Set ourselves as the process's attached object. */
                m_old_process_state = target->SetDebugObject(this);

                /* Send an event for our attaching to the process. */
                this->PushDebugEvent(ams::svc::DebugEvent_CreateProcess, nullptr, 0);

                /* Send events for attaching to each thread in the process. */
                {
                    auto end = target->GetThreadList().end();
                    for (auto it = target->GetThreadList().begin(); it != end; ++it) {
                        /* Request that we suspend the thread. */
                        it->RequestSuspend(KThread::SuspendType_Debug);

                        /* If the thread is in a state for us to do so, generate the event. */
                        if (const auto thread_state = it->GetState(); thread_state == KThread::ThreadState_Runnable || thread_state == KThread::ThreadState_Waiting) {
                            /* Mark the thread as attached to. */
                            it->SetDebugAttached();

                            /* Send the event. */
                            const uintptr_t params[2] = { it->GetId(), GetInteger(it->GetThreadLocalRegionAddress()) };
                            this->PushDebugEvent(ams::svc::DebugEvent_CreateThread, params, util::size(params));
                        }
                    }
                }

                /* Send the process's jit debug info, if relevant. */
                if (KEventInfo *jit_info = target->GetJitDebugInfo(); jit_info != nullptr) {
                    this->EnqueueDebugEventInfo(jit_info);
                }

                /* Send an exception event to represent our attaching. */
                const uintptr_t params[1] = { static_cast<uintptr_t>(ams::svc::DebugException_DebuggerAttached) };
                this->PushDebugEvent(ams::svc::DebugEvent_Exception, params, util::size(params));

                /* Signal. */
                this->NotifyAvailable();
            }
        }

        R_SUCCEED();
    }

    Result KDebugBase::BreakProcess() {
        /* Check that we're attached. */
        R_UNLESS(this->IsAttached(), svc::ResultProcessTerminated());

        /* Open a reference to our process. */
        R_UNLESS(this->OpenProcess(), svc::ResultProcessTerminated());

        /* Close our reference to our process when we're done. */
        ON_SCOPE_EXIT { this->CloseProcess(); };

        /* Get the process pointer. */
        KProcess * const target = this->GetProcessUnsafe();

        /* Lock both ourselves, the target process, and the scheduler. */
        KScopedLightLock state_lk(target->GetStateLock());
        KScopedLightLock list_lk(target->GetListLock());
        KScopedLightLock this_lk(m_lock);
        KScopedSchedulerLock sl;

        /* Check that we're still attached now that we're locked. */
        R_UNLESS(this->IsAttached(), svc::ResultProcessTerminated());

        /* Check that the process isn't terminated. */
        R_UNLESS(!target->IsTerminated(), svc::ResultProcessTerminated());

        /* Get the currently active threads. */
        constexpr u64 ThreadIdNoThread      = -1ll;
        constexpr u64 ThreadIdUnknownThread = -2ll;
        uintptr_t debug_info_params[1 + cpu::NumCores] = { static_cast<uintptr_t>(ams::svc::DebugException_DebuggerBreak), };
        for (size_t i = 0; i < cpu::NumCores; ++i) {
            /* Get the currently running thread. */
            KThread *thread = target->GetRunningThread(i);

            /* Check that the thread's idle count is correct. */
            if (target->GetRunningThreadIdleCount(i) == Kernel::GetScheduler(i).GetIdleCount()) {
                if (thread != nullptr && static_cast<size_t>(thread->GetActiveCore()) == i) {
                    debug_info_params[1 + i] = thread->GetId();
                } else {
                    /* We found an unknown thread. */
                    debug_info_params[1 + i] = ThreadIdUnknownThread;
                }
            } else {
                /* We didn't find a thread. */
                debug_info_params[1 + i] = ThreadIdNoThread;
            }
        }

        /* Suspend all the threads in the process. */
        {
            auto end = target->GetThreadList().end();
            for (auto it = target->GetThreadList().begin(); it != end; ++it) {
                /* Request that we suspend the thread. */
                it->RequestSuspend(KThread::SuspendType_Debug);
            }
        }

        /* Send an exception event to represent our breaking the process. */
        this->PushDebugEvent(ams::svc::DebugEvent_Exception, debug_info_params, util::size(debug_info_params));

        /* Signal. */
        this->NotifyAvailable();

        /* Set the process as breaked. */
        target->SetDebugBreak();

        R_SUCCEED();
    }

    Result KDebugBase::TerminateProcess() {
        /* Check that we're attached. */
        R_UNLESS(this->IsAttached(), ResultSuccess());

        /* Open a reference to our process. */
        R_UNLESS(this->OpenProcess(), ResultSuccess());

        /* Close our reference to our process when we're done. */
        ON_SCOPE_EXIT { this->CloseProcess(); };

        /* Get the process pointer. */
        KProcess * const target = this->GetProcessUnsafe();

        /* Terminate the process. */
        /* NOTE: This result is seemingly-intentionally not checked by Nintendo. */
        static_cast<void>(target->Terminate());

        R_SUCCEED();
    }

    Result KDebugBase::GetThreadContext(ams::svc::ThreadContext *out, u64 thread_id, u32 context_flags) {
        /* Check that we're attached. */
        R_UNLESS(this->IsAttached(), svc::ResultProcessTerminated());

        /* Open a reference to our process. */
        R_UNLESS(this->OpenProcess(), svc::ResultProcessTerminated());

        /* Close our reference to our process when we're done. */
        ON_SCOPE_EXIT { this->CloseProcess(); };

        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Check that we're still attached now that we're locked. */
        R_UNLESS(this->IsAttached(), svc::ResultProcessTerminated());

        /* Get the process pointer. */
        KProcess * const process = this->GetProcessUnsafe();

        /* Get the thread from its id. */
        KThread *thread = KThread::GetThreadFromId(thread_id);
        R_UNLESS(thread != nullptr, svc::ResultInvalidId());
        ON_SCOPE_EXIT { thread->Close(); };

        /* Verify that the thread is owned by our process. */
        R_UNLESS(process == thread->GetOwnerProcess(), svc::ResultInvalidId());

        /* Verify that the thread isn't terminated. */
        R_UNLESS(thread->GetState() != KThread::ThreadState_Terminated, svc::ResultTerminationRequested());

        /* Check that the thread is not the current one. */
        /* NOTE: Nintendo does not check this, and thus the following loop will deadlock. */
        R_UNLESS(thread != GetCurrentThreadPointer(), svc::ResultInvalidId());

        /* Try to get the thread context until the thread isn't current on any core. */
        while (true) {
            KScopedSchedulerLock sl;

            /* The thread needs to be requested for debug suspension. */
            R_UNLESS(thread->IsSuspendRequested(KThread::SuspendType_Debug), svc::ResultInvalidState());

            /* If the thread's raw state isn't runnable, check if it's current on some core. */
            if (thread->GetRawState() != KThread::ThreadState_Runnable) {
                bool current = false;
                for (auto i = 0; i < static_cast<s32>(cpu::NumCores); ++i) {
                    if (thread == Kernel::GetScheduler(i).GetSchedulerCurrentThread()) {
                        current = true;
                        break;
                    }
                }

                /* If the thread is current, retry until it isn't. */
                if (current) {
                    continue;
                }
            }

            /* Get the thread context. */
            static_assert(std::derived_from<KDebug, KDebugBase>);
            R_RETURN(static_cast<KDebug *>(this)->GetThreadContextImpl(out, thread, context_flags));
        }
    }

    Result KDebugBase::SetThreadContext(const ams::svc::ThreadContext &ctx, u64 thread_id, u32 context_flags) {
        /* Check that we're attached. */
        R_UNLESS(this->IsAttached(), svc::ResultProcessTerminated());

        /* Open a reference to our process. */
        R_UNLESS(this->OpenProcess(), svc::ResultProcessTerminated());

        /* Close our reference to our process when we're done. */
        ON_SCOPE_EXIT { this->CloseProcess(); };

        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Check that we're still attached now that we're locked. */
        R_UNLESS(this->IsAttached(), svc::ResultProcessTerminated());

        /* Get the process pointer. */
        KProcess * const process = this->GetProcessUnsafe();

        /* Get the thread from its id. */
        KThread *thread = KThread::GetThreadFromId(thread_id);
        R_UNLESS(thread != nullptr, svc::ResultInvalidId());
        ON_SCOPE_EXIT { thread->Close(); };

        /* Verify that the thread is owned by our process. */
        R_UNLESS(process == thread->GetOwnerProcess(), svc::ResultInvalidId());

        /* Verify that the thread isn't terminated. */
        R_UNLESS(thread->GetState() != KThread::ThreadState_Terminated, svc::ResultTerminationRequested());

        /* Check that the thread is not the current one. */
        /* NOTE: Nintendo does not check this, and thus the following loop will deadlock. */
        R_UNLESS(thread != GetCurrentThreadPointer(), svc::ResultInvalidId());

        /* Try to get the thread context until the thread isn't current on any core. */
        while (true) {
            KScopedSchedulerLock sl;

            /* The thread needs to be requested for debug suspension. */
            R_UNLESS(thread->IsSuspendRequested(KThread::SuspendType_Debug), svc::ResultInvalidState());

            /* If the thread's raw state isn't runnable, check if it's current on some core. */
            if (thread->GetRawState() != KThread::ThreadState_Runnable) {
                bool current = false;
                for (auto i = 0; i < static_cast<s32>(cpu::NumCores); ++i) {
                    if (thread == Kernel::GetScheduler(i).GetSchedulerCurrentThread()) {
                        current = true;
                        break;
                    }
                }

                /* If the thread is current, retry until it isn't. */
                if (current) {
                    continue;
                }
            }

            /* Update thread single-step state. */
            #if defined(MESOSPHERE_ENABLE_HARDWARE_SINGLE_STEP)
            {
                if ((context_flags & ams::svc::ThreadContextFlag_SetSingleStep) != 0) {
                    /* Set single step. */
                    thread->SetHardwareSingleStep();

                    /* If no other thread flags are present, we're done. */
                    R_SUCCEED_IF((context_flags & ~ams::svc::ThreadContextFlag_SetSingleStep) == 0);
                } else if ((context_flags & ams::svc::ThreadContextFlag_ClearSingleStep) != 0) {
                    /* Clear single step. */
                    thread->ClearHardwareSingleStep();

                    /* If no other thread flags are present, we're done. */
                    R_SUCCEED_IF((context_flags & ~ams::svc::ThreadContextFlag_ClearSingleStep) == 0);
                }
            }
            #endif

            /* Verify that the thread's svc state is valid. */
            if (thread->IsCallingSvc()) {
                const u8 svc_id         = thread->GetSvcId();

                const bool is_valid_svc = svc_id == svc::SvcId_Break ||
                                          svc_id == svc::SvcId_ReturnFromException;

                R_UNLESS(is_valid_svc, svc::ResultInvalidState());
            }

            /* Set the thread context. */
            static_assert(std::derived_from<KDebug, KDebugBase>);
            R_RETURN(static_cast<KDebug *>(this)->SetThreadContextImpl(ctx, thread, context_flags));
        }
    }


    Result KDebugBase::ContinueDebug(const u32 flags, const u64 *thread_ids, size_t num_thread_ids) {
        /* Check that we're attached. */
        R_UNLESS(this->IsAttached(), svc::ResultProcessTerminated());

        /* Open a reference to our process. */
        R_UNLESS(this->OpenProcess(), svc::ResultProcessTerminated());

        /* Close our reference to our process when we're done. */
        ON_SCOPE_EXIT { this->CloseProcess(); };

        /* Get the process pointer. */
        KProcess * const target = this->GetProcessUnsafe();

        /* Lock both ourselves, the target process, and the scheduler. */
        KScopedLightLock state_lk(target->GetStateLock());
        KScopedLightLock list_lk(target->GetListLock());
        KScopedLightLock this_lk(m_lock);
        KScopedSchedulerLock sl;

        /* Check that we're still attached now that we're locked. */
        R_UNLESS(this->IsAttached(), svc::ResultProcessTerminated());

        /* Check that the process isn't terminated. */
        R_UNLESS(!target->IsTerminated(), svc::ResultProcessTerminated());

        /* Check that we have no pending events. */
        R_UNLESS(m_event_info_list.empty(), svc::ResultBusy());

        /* Clear the target's JIT debug info. */
        target->ClearJitDebugInfo();

        /* Set our continue flags. */
        m_continue_flags = flags;

        /* Iterate over threads, continuing them as we should. */
        bool has_debug_break_thread = false;
        {
            /* Parse our flags. */
            const bool exception_handled = (m_continue_flags & ams::svc::ContinueFlag_ExceptionHandled) != 0;
            const bool continue_all      = (m_continue_flags & ams::svc::ContinueFlag_ContinueAll)      != 0;
            const bool continue_others   = (m_continue_flags & ams::svc::ContinueFlag_ContinueOthers)   != 0;

            /* Update each thread. */
            auto end = target->GetThreadList().end();
            for (auto it = target->GetThreadList().begin(); it != end; ++it) {
                /* Determine if we should continue the thread. */
                bool should_continue;
                {
                    if (continue_all) {
                        /* Continue all threads. */
                        should_continue = true;
                    } else if (continue_others) {
                        /* Continue the thread if it doesn't match one of our target ids. */
                        const u64 thread_id = it->GetId();
                        should_continue = true;
                        for (size_t i = 0; i < num_thread_ids; ++i) {
                            if (thread_ids[i] == thread_id) {
                                should_continue = false;
                                break;
                            }
                        }
                    } else {
                        /* Continue the thread if it matches one of our target ids. */
                        const u64 thread_id = it->GetId();
                        should_continue = false;
                        for (size_t i = 0; i < num_thread_ids; ++i) {
                            if (thread_ids[i] == thread_id) {
                                should_continue = true;
                                break;
                            }
                        }
                    }
                }

                /* Continue the thread if we should. */
                if (should_continue) {
                    if (exception_handled) {
                        it->SetDebugExceptionResult(svc::ResultStopProcessingException());
                    }
                    it->Resume(KThread::SuspendType_Debug);
                }

                /* If the thread has debug suspend requested, note so. */
                if (it->IsSuspendRequested(KThread::SuspendType_Debug)) {
                    has_debug_break_thread = true;
                }
            }
        }

        /* Set the process's state. */
        if (has_debug_break_thread) {
            target->SetDebugBreak();
        } else {
            target->SetAttached();
        }

        R_SUCCEED();
    }

    KEventInfo *KDebugBase::CreateDebugEvent(ams::svc::DebugEvent event, u64 cur_thread_id, const uintptr_t *params, size_t num_params) {
        /* Allocate a new event. */
        KEventInfo *info = KEventInfo::Allocate();

        /* Populate the event info. */
        if (info != nullptr) {
            /* Set common fields. */
            info->event     = event;
            info->thread_id = 0;
            info->flags     = ams::svc::DebugEventFlag_Stopped;

            /* Set event specific fields. */
            switch (event) {
                case ams::svc::DebugEvent_CreateProcess:
                    {
                        /* Check parameters. */
                        MESOSPHERE_ASSERT(params == nullptr);
                        MESOSPHERE_ASSERT(num_params == 0);
                    }
                    break;
                case ams::svc::DebugEvent_CreateThread:
                    {
                        /* Check parameters. */
                        MESOSPHERE_ASSERT(params != nullptr);
                        MESOSPHERE_ASSERT(num_params == 2);

                        /* Set the thread id. */
                        info->thread_id = params[0];

                        /* Set the thread creation info. */
                        info->info.create_thread.thread_id   = params[0];
                        info->info.create_thread.tls_address = params[1];
                    }
                    break;
                case ams::svc::DebugEvent_ExitProcess:
                    {
                        /* Check parameters. */
                        MESOSPHERE_ASSERT(params != nullptr);
                        MESOSPHERE_ASSERT(num_params == 1);

                        /* Set the exit reason. */
                        info->info.exit_process.reason = static_cast<ams::svc::ProcessExitReason>(params[0]);

                        /* Clear the thread id and flags. */
                        info->thread_id = 0;
                        info->flags = 0;
                    }
                    break;
                case ams::svc::DebugEvent_ExitThread:
                    {
                        /* Check parameters. */
                        MESOSPHERE_ASSERT(params != nullptr);
                        MESOSPHERE_ASSERT(num_params == 2);

                        /* Set the thread id. */
                        info->thread_id = params[0];

                        /* Set the exit reason. */
                        info->info.exit_thread.reason = static_cast<ams::svc::ThreadExitReason>(params[1]);
                    }
                    break;
                case ams::svc::DebugEvent_Exception:
                    {
                        /* Check parameters. */
                        MESOSPHERE_ASSERT(params != nullptr);
                        MESOSPHERE_ASSERT(num_params >= 1);

                        /* Set the thread id. */
                        info->thread_id = cur_thread_id;

                        /* Set the exception type, and clear the count. */
                        info->info.exception.exception_type       = static_cast<ams::svc::DebugException>(params[0]);
                        info->info.exception.exception_data_count = 0;
                        switch (static_cast<ams::svc::DebugException>(params[0])) {
                            case ams::svc::DebugException_UndefinedInstruction:
                            case ams::svc::DebugException_BreakPoint:
                            case ams::svc::DebugException_UndefinedSystemCall:
                                {
                                    MESOSPHERE_ASSERT(num_params >= 3);

                                    info->info.exception.exception_address    = params[1];

                                    info->info.exception.exception_data_count = 1;
                                    info->info.exception.exception_data[0]    = params[2];
                                }
                                break;
                            case ams::svc::DebugException_DebuggerAttached:
                                {
                                    info->thread_id = 0;

                                    info->info.exception.exception_address = 0;
                                }
                                break;
                            case ams::svc::DebugException_UserBreak:
                                {
                                    MESOSPHERE_ASSERT(num_params >= 2);

                                    info->info.exception.exception_address    = params[1];

                                    info->info.exception.exception_data_count = 0;
                                    for (size_t i = 2; i < num_params; ++i) {
                                        info->info.exception.exception_data[info->info.exception.exception_data_count++] = params[i];
                                    }
                                }
                                break;
                            case ams::svc::DebugException_DebuggerBreak:
                                {
                                    info->thread_id = 0;

                                    info->info.exception.exception_address    = 0;

                                    info->info.exception.exception_data_count = 0;
                                    for (size_t i = 1; i < num_params; ++i) {
                                        info->info.exception.exception_data[info->info.exception.exception_data_count++] = params[i];
                                    }
                                }
                                break;
                            case ams::svc::DebugException_MemorySystemError:
                                {
                                    info->info.exception.exception_address = 0;
                                }
                                break;
                            case ams::svc::DebugException_InstructionAbort:
                            case ams::svc::DebugException_DataAbort:
                            case ams::svc::DebugException_AlignmentFault:
                            default:
                                {
                                    MESOSPHERE_ASSERT(num_params >= 2);

                                    info->info.exception.exception_address = params[1];
                                }
                                break;
                        }
                    }
                    break;
            }
        }

        return info;
    }

    void KDebugBase::PushDebugEvent(ams::svc::DebugEvent event, const uintptr_t *params, size_t num_params) {
        /* Create and enqueue and event. */
        if (KEventInfo *new_info = CreateDebugEvent(event, GetCurrentThread().GetId(), params, num_params); new_info != nullptr) {
            this->EnqueueDebugEventInfo(new_info);
        }
    }

    void KDebugBase::EnqueueDebugEventInfo(KEventInfo *info) {
        /* Lock the scheduler. */
        KScopedSchedulerLock sl;

        /* Push the event to the back of the list. */
        m_event_info_list.push_back(*info);
    }


    template<typename T> requires (std::same_as<T, ams::svc::lp64::DebugEventInfo> || std::same_as<T, ams::svc::ilp32::DebugEventInfo>)
    Result KDebugBase::GetDebugEventInfoImpl(T *out) {
        /* Check that we're attached. */
        R_UNLESS(this->IsAttached(), svc::ResultProcessTerminated());

        /* Open a reference to our process. */
        R_UNLESS(this->OpenProcess(), svc::ResultProcessTerminated());

        /* Close our reference to our process when we're done. */
        ON_SCOPE_EXIT { this->CloseProcess(); };

        /* Get the process pointer. */
        KProcess * const process = this->GetProcessUnsafe();

        /* Pop an event info from our queue. */
        KEventInfo *info = nullptr;
        {
            KScopedSchedulerLock sl;

            /* Check that we have an event to dequeue. */
            R_UNLESS(!m_event_info_list.empty(), svc::ResultNoEvent());

            /* Pop the event from the front of the queue. */
            info = std::addressof(m_event_info_list.front());
            m_event_info_list.pop_front();
        }
        MESOSPHERE_ASSERT(info != nullptr);

        /* Free the event info once we're done with it. */
        ON_SCOPE_EXIT { KEventInfo::Free(info); };

        /* Set common fields. */
        out->type      = info->event;
        out->thread_id = info->thread_id;
        out->flags     = info->flags;

        /* Set event specific fields. */
        switch (info->event) {
            case ams::svc::DebugEvent_CreateProcess:
                {
                    out->info.create_process.program_id                     = process->GetProgramId();
                    out->info.create_process.process_id                     = process->GetId();
                    out->info.create_process.flags                          = process->GetCreateProcessFlags();
                    out->info.create_process.user_exception_context_address = GetInteger(process->GetProcessLocalRegionAddress());

                    std::memcpy(out->info.create_process.name, process->GetName(), sizeof(out->info.create_process.name));
                }
                break;
            case ams::svc::DebugEvent_CreateThread:
                {
                    out->info.create_thread.thread_id   = info->info.create_thread.thread_id;
                    out->info.create_thread.tls_address = info->info.create_thread.tls_address;
                }
                break;
            case ams::svc::DebugEvent_ExitProcess:
                {
                    out->info.exit_process.reason = info->info.exit_process.reason;
                }
                break;
            case ams::svc::DebugEvent_ExitThread:
                {
                    out->info.exit_thread.reason = info->info.exit_thread.reason;
                }
                break;
            case ams::svc::DebugEvent_Exception:
                {
                    out->info.exception.type    = info->info.exception.exception_type;
                    out->info.exception.address = info->info.exception.exception_address;

                    switch (info->info.exception.exception_type) {
                        case ams::svc::DebugException_UndefinedInstruction:
                            {
                                MESOSPHERE_ASSERT(info->info.exception.exception_data_count == 1);
                                /* Only save the instruction if the caller is not force debug prod. */
                                if (this->IsForceDebugProd()) {
                                    out->info.exception.specific.undefined_instruction.insn = 0;
                                } else {
                                    out->info.exception.specific.undefined_instruction.insn = info->info.exception.exception_data[0];
                                }
                            }
                            break;
                        case ams::svc::DebugException_BreakPoint:
                            {
                                MESOSPHERE_ASSERT(info->info.exception.exception_data_count == 1);
                                out->info.exception.specific.break_point.type    = static_cast<ams::svc::BreakPointType>(info->info.exception.exception_data[0]);
                                out->info.exception.specific.break_point.address = 0;
                            }
                            break;
                        case ams::svc::DebugException_UserBreak:
                            {
                                MESOSPHERE_ASSERT(info->info.exception.exception_data_count == 3);
                                out->info.exception.specific.user_break.break_reason = static_cast<ams::svc::BreakReason>(info->info.exception.exception_data[0]);
                                out->info.exception.specific.user_break.address      = info->info.exception.exception_data[1];
                                out->info.exception.specific.user_break.size         = info->info.exception.exception_data[2];
                            }
                            break;
                        case ams::svc::DebugException_DebuggerBreak:
                            {
                                /* TODO: How does this work with non-4 cpu count? */
                                static_assert(cpu::NumCores <= 4);

                                MESOSPHERE_ASSERT(info->info.exception.exception_data_count == cpu::NumCores);
                                out->info.exception.specific.debugger_break.active_thread_ids[0] = info->info.exception.exception_data[0];
                                out->info.exception.specific.debugger_break.active_thread_ids[1] = info->info.exception.exception_data[1];
                                out->info.exception.specific.debugger_break.active_thread_ids[2] = info->info.exception.exception_data[2];
                                out->info.exception.specific.debugger_break.active_thread_ids[3] = info->info.exception.exception_data[3];
                            }
                            break;
                        case ams::svc::DebugException_UndefinedSystemCall:
                            {
                                MESOSPHERE_ASSERT(info->info.exception.exception_data_count == 1);
                                out->info.exception.specific.undefined_system_call.id = info->info.exception.exception_data[0];
                            }
                            break;
                        default:
                            {
                                /* ... */
                            }
                            break;
                    }
                }
                break;
        }

        R_SUCCEED();
    }

    Result KDebugBase::GetDebugEventInfo(ams::svc::lp64::DebugEventInfo *out) {
        R_RETURN(this->GetDebugEventInfoImpl(out));
    }

    Result KDebugBase::GetDebugEventInfo(ams::svc::ilp32::DebugEventInfo *out) {
        R_RETURN(this->GetDebugEventInfoImpl(out));
    }

    void KDebugBase::Finalize() {
        /* Perform base finalization. */
        KSynchronizationObject::Finalize();

        /* Perform post-synchronization finalization. */
        this->OnFinalizeSynchronizationObject();
    }

    void KDebugBase::OnFinalizeSynchronizationObject() {
        /* Detach from our process, if we have one. */
        if (this->IsAttached() && this->OpenProcess()) {
            /* Close the process when we're done with it. */
            ON_SCOPE_EXIT { this->CloseProcess(); };

            /* Get the process pointer. */
            KProcess * const process = this->GetProcessUnsafe();

            /* Lock both ourselves and the target process. */
            KScopedLightLock state_lk(process->GetStateLock());
            KScopedLightLock list_lk(process->GetListLock());
            KScopedLightLock this_lk(m_lock);

            /* Check that we're still attached. */
            if (m_is_attached) {
                KScopedSchedulerLock sl;

                /* Detach ourselves from the process. */
                process->ClearDebugObject(m_old_process_state);

                /* Release all threads. */
                const bool resume = (process->GetState() != KProcess::State_Crashed);
                {
                    auto end = process->GetThreadList().end();
                    for (auto it = process->GetThreadList().begin(); it != end; ++it) {
                        #if defined(MESOSPHERE_ENABLE_HARDWARE_SINGLE_STEP)
                        /* Clear the thread's single-step state. */
                        it->ClearHardwareSingleStep();
                        #endif

                        if (resume) {
                            /* If the process isn't crashed, resume threads. */
                            it->Resume(KThread::SuspendType_Debug);
                        } else {
                            /* Otherwise, suspend them. */
                            it->RequestSuspend(KThread::SuspendType_Debug);
                        }
                    }
                }

                /* Note we're now unattached. */
                m_is_attached = false;

                /* Close the initial reference opened to our process. */
                this->CloseProcess();
            }
        }

        /* Free any pending events. */
        {
            KScopedSchedulerLock sl;

            while (!m_event_info_list.empty()) {
                KEventInfo *info = std::addressof(m_event_info_list.front());
                m_event_info_list.pop_front();
                KEventInfo::Free(info);
            }
        }
    }

    bool KDebugBase::IsSignaled() const {
        bool empty;
        {
            KScopedSchedulerLock sl;

            empty = m_event_info_list.empty();
        }

        return !empty || !m_is_attached || this->GetProcessUnsafe()->IsTerminated();
    }

    Result KDebugBase::ProcessDebugEvent(ams::svc::DebugEvent event, const uintptr_t *params, size_t num_params) {
        /* Get the current process. */
        KProcess *process = GetCurrentProcessPointer();

        /* If the event is CreateThread and we've already attached, there's nothing to do. */
        if (event == ams::svc::DebugEvent_CreateThread) {
            R_SUCCEED_IF(GetCurrentThread().IsAttachedToDebugger());
        }

        while (true) {
            /* Lock the process and the scheduler. */
            KScopedLightLock state_lk(process->GetStateLock());
            KScopedLightLock list_lk(process->GetListLock());
            KScopedSchedulerLock sl;

            /* If the current thread is terminating, we can't process an event. */
            R_SUCCEED_IF(GetCurrentThread().IsTerminationRequested());

            /* Get the debug object. If we have none, there's nothing to process. */
            KDebugBase *debug = GetDebugObject(process);
            R_SUCCEED_IF(debug == nullptr);

            /* If the event is an exception and we don't have exception events enabled, we can't handle the event. */
            if (event == ams::svc::DebugEvent_Exception && (debug->m_continue_flags & ams::svc::ContinueFlag_EnableExceptionEvent) == 0) {
                GetCurrentThread().SetDebugExceptionResult(ResultSuccess());
                R_THROW(svc::ResultNotHandled());
            }

            /* If the current thread is suspended, retry. */
            if (GetCurrentThread().IsSuspended()) {
                continue;
            }

            /* Suspend all the process's threads. */
            {
                auto end = process->GetThreadList().end();
                for (auto it = process->GetThreadList().begin(); it != end; ++it) {
                    it->RequestSuspend(KThread::SuspendType_Debug);
                }
            }

            /* Push the event. */
            debug->PushDebugEvent(event, params, num_params);
            debug->NotifyAvailable();

            /* Set the process as breaked. */
            process->SetDebugBreak();

            /* If the event is an exception, set the result and clear single step. */
            if (event == ams::svc::DebugEvent_Exception) {
                GetCurrentThread().SetDebugExceptionResult(ResultSuccess());
            }

            /* Exit our retry loop. */
            break;
        }

        /* If the event is an exception, get the exception result. */
        if (event == ams::svc::DebugEvent_Exception) {
            /* Lock the scheduler. */
            KScopedSchedulerLock sl;

            /* If the thread is terminating, we can't process the exception. */
            R_UNLESS(!GetCurrentThread().IsTerminationRequested(), svc::ResultStopProcessingException());

            /* Get the debug object. */
            if (KDebugBase *debug = GetDebugObject(process); debug != nullptr) {
                /* If we have one, check the debug exception. */
                R_RETURN(GetCurrentThread().GetDebugExceptionResult());
            } else {
                /* We don't have a debug object, so stop processing the exception. */
                R_THROW(svc::ResultStopProcessingException());
            }
        }

        R_SUCCEED();
    }

    Result KDebugBase::OnDebugEvent(ams::svc::DebugEvent event, const uintptr_t *params, size_t num_params) {
        if (KProcess *process = GetCurrentProcessPointer(); process != nullptr && process->IsAttachedToDebugger()) {
            R_RETURN(ProcessDebugEvent(event, params, num_params));
        }
        R_SUCCEED();
    }

    void KDebugBase::OnExitProcess(KProcess *process) {
        MESOSPHERE_ASSERT(process != nullptr);

        /* Check if we're attached to a debugger. */
        if (process->IsAttachedToDebugger()) {
            /* If we are, lock the scheduler. */
            KScopedSchedulerLock sl;

            /* Push the event. */
            if (KDebugBase *debug = GetDebugObject(process); debug != nullptr) {
                const uintptr_t params[1] = { static_cast<uintptr_t>(ams::svc::ProcessExitReason_ExitProcess) };
                debug->PushDebugEvent(ams::svc::DebugEvent_ExitProcess, params, util::size(params));
                debug->NotifyAvailable();
            }
        }
    }

    void KDebugBase::OnTerminateProcess(KProcess *process) {
        MESOSPHERE_ASSERT(process != nullptr);

        /* Check if we're attached to a debugger. */
        if (process->IsAttachedToDebugger()) {
            /* If we are, lock the scheduler. */
            KScopedSchedulerLock sl;

            /* Push the event. */
            if (KDebugBase *debug = GetDebugObject(process); debug != nullptr) {
                const uintptr_t params[1] = { static_cast<uintptr_t>(ams::svc::ProcessExitReason_TerminateProcess) };
                debug->PushDebugEvent(ams::svc::DebugEvent_ExitProcess, params, util::size(params));
                debug->NotifyAvailable();
            }
        }
    }

    void KDebugBase::OnExitThread(KThread *thread) {
        MESOSPHERE_ASSERT(thread != nullptr);

        /* Check if we're attached to a debugger. */
        if (KProcess *process = thread->GetOwnerProcess(); process != nullptr && process->IsAttachedToDebugger()) {
            /* If we are, submit the event. */
            const uintptr_t params[2] = { thread->GetId(), static_cast<uintptr_t>(thread->IsTerminationRequested() ? ams::svc::ThreadExitReason_TerminateThread : ams::svc::ThreadExitReason_ExitThread) };
            static_cast<void>(OnDebugEvent(ams::svc::DebugEvent_ExitThread, params, util::size(params)));
        }
    }

}
