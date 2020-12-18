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

        ALWAYS_INLINE KDebugBase *GetDebugObject(KProcess *process) {
            return static_cast<KDebugBase *>(process->GetDebugObject());
        }

    }

    void KDebugBase::Initialize() {
        /* Clear the process and continue flags. */
        m_process        = nullptr;
        m_continue_flags = 0;
    }

    bool KDebugBase::Is64Bit() const {
        MESOSPHERE_ASSERT(m_lock.IsLockedByCurrentThread());
        MESOSPHERE_ASSERT(m_process != nullptr);
        return m_process->Is64Bit();
    }


    Result KDebugBase::QueryMemoryInfo(ams::svc::MemoryInfo *out_memory_info, ams::svc::PageInfo *out_page_info, KProcessAddress address) {
        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Check that we have a valid process. */
        R_UNLESS(m_process != nullptr,       svc::ResultProcessTerminated());
        R_UNLESS(!m_process->IsTerminated(), svc::ResultProcessTerminated());

        /* Query the mapping's info. */
        KMemoryInfo info;
        R_TRY(m_process->GetPageTable().QueryInfo(std::addressof(info), out_page_info, address));

        /* Write output. */
        *out_memory_info = info.GetSvcMemoryInfo();
        return ResultSuccess();
    }

    Result KDebugBase::ReadMemory(KProcessAddress buffer, KProcessAddress address, size_t size) {
        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Check that we have a valid process. */
        R_UNLESS(m_process != nullptr,       svc::ResultProcessTerminated());
        R_UNLESS(!m_process->IsTerminated(), svc::ResultProcessTerminated());

        /* Get the page tables. */
        KProcessPageTable &debugger_pt = GetCurrentProcess().GetPageTable();
        KProcessPageTable &target_pt   = m_process->GetPageTable();

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
            if (info.GetState() != KMemoryState_Io) {
                /* The memory is normal memory. */
                R_TRY(target_pt.ReadDebugMemory(GetVoidPointer(buffer), cur_address, cur_size));
            } else {
                /* The memory is IO memory. */

                /* Verify that the memory is readable. */
                R_UNLESS((info.GetPermission() & KMemoryPermission_UserRead) == KMemoryPermission_UserRead, svc::ResultInvalidAddress());

                /* Get the physical address of the memory.  */
                /* NOTE: Nintendo does not verify the result of this call. */
                KPhysicalAddress phys_addr;
                target_pt.GetPhysicalAddress(std::addressof(phys_addr), cur_address);

                /* Map the address as IO in the current process. */
                R_TRY(debugger_pt.MapIo(util::AlignDown(GetInteger(phys_addr), PageSize), PageSize, KMemoryPermission_UserRead));

                /* Get the address of the newly mapped IO. */
                KProcessAddress io_address;
                Result query_result = debugger_pt.QueryIoMapping(std::addressof(io_address), util::AlignDown(GetInteger(phys_addr), PageSize), PageSize);
                MESOSPHERE_R_ASSERT(query_result);
                R_TRY(query_result);

                /* Ensure we clean up the new mapping on scope exit. */
                ON_SCOPE_EXIT { MESOSPHERE_R_ABORT_UNLESS(debugger_pt.UnmapPages(util::AlignDown(GetInteger(io_address), PageSize), 1, KMemoryState_Io)); };

                /* Adjust the io address for alignment. */
                io_address += (GetInteger(cur_address) & (PageSize - 1));

                /* Get the readable size. */
                const size_t readable_size = std::min(cur_size, util::AlignDown(GetInteger(cur_address) + PageSize, PageSize) - GetInteger(cur_address));

                /* Read the memory. */
                switch ((GetInteger(cur_address) | readable_size) & 3) {
                    case 0:
                        {
                            R_UNLESS(UserspaceAccess::ReadIoMemory32Bit(GetVoidPointer(buffer), GetVoidPointer(io_address), readable_size), svc::ResultInvalidPointer());
                        }
                        break;
                    case 2:
                        {
                            R_UNLESS(UserspaceAccess::ReadIoMemory16Bit(GetVoidPointer(buffer), GetVoidPointer(io_address), readable_size), svc::ResultInvalidPointer());
                        }
                        break;
                    default:
                        {
                            R_UNLESS(UserspaceAccess::ReadIoMemory8Bit(GetVoidPointer(buffer), GetVoidPointer(io_address), readable_size),  svc::ResultInvalidPointer());
                        }
                        break;
                }
            }

            /* Advance. */
            buffer      += cur_size;
            cur_address += cur_size;
            remaining   -= cur_size;
        }

        return ResultSuccess();
    }

    Result KDebugBase::WriteMemory(KProcessAddress buffer, KProcessAddress address, size_t size) {
        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Check that we have a valid process. */
        R_UNLESS(m_process != nullptr,       svc::ResultProcessTerminated());
        R_UNLESS(!m_process->IsTerminated(), svc::ResultProcessTerminated());

        /* Get the page tables. */
        KProcessPageTable &debugger_pt = GetCurrentProcess().GetPageTable();
        KProcessPageTable &target_pt   = m_process->GetPageTable();

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
            if (info.GetState() != KMemoryState_Io) {
                /* The memory is normal memory. */
                R_TRY(target_pt.WriteDebugMemory(cur_address, GetVoidPointer(buffer), cur_size));
            } else {
                /* The memory is IO memory. */

                /* Verify that the memory is writable. */
                R_UNLESS((info.GetPermission() & KMemoryPermission_UserReadWrite) == KMemoryPermission_UserReadWrite, svc::ResultInvalidAddress());

                /* Get the physical address of the memory.  */
                /* NOTE: Nintendo does not verify the result of this call. */
                KPhysicalAddress phys_addr;
                target_pt.GetPhysicalAddress(std::addressof(phys_addr), cur_address);

                /* Map the address as IO in the current process. */
                R_TRY(debugger_pt.MapIo(util::AlignDown(GetInteger(phys_addr), PageSize), PageSize, KMemoryPermission_UserReadWrite));

                /* Get the address of the newly mapped IO. */
                KProcessAddress io_address;
                Result query_result = debugger_pt.QueryIoMapping(std::addressof(io_address), util::AlignDown(GetInteger(phys_addr), PageSize), PageSize);
                MESOSPHERE_R_ASSERT(query_result);
                R_TRY(query_result);

                /* Ensure we clean up the new mapping on scope exit. */
                ON_SCOPE_EXIT { MESOSPHERE_R_ABORT_UNLESS(debugger_pt.UnmapPages(util::AlignDown(GetInteger(io_address), PageSize), 1, KMemoryState_Io)); };

                /* Adjust the io address for alignment. */
                io_address += (GetInteger(cur_address) & (PageSize - 1));

                /* Get the readable size. */
                const size_t readable_size = std::min(cur_size, util::AlignDown(GetInteger(cur_address) + PageSize, PageSize) - GetInteger(cur_address));

                /* Read the memory. */
                switch ((GetInteger(cur_address) | readable_size) & 3) {
                    case 0:
                        {
                            R_UNLESS(UserspaceAccess::WriteIoMemory32Bit(GetVoidPointer(io_address), GetVoidPointer(buffer), readable_size), svc::ResultInvalidPointer());
                        }
                        break;
                    case 2:
                        {
                            R_UNLESS(UserspaceAccess::WriteIoMemory16Bit(GetVoidPointer(io_address), GetVoidPointer(buffer), readable_size), svc::ResultInvalidPointer());
                        }
                        break;
                    default:
                        {
                            R_UNLESS(UserspaceAccess::WriteIoMemory8Bit(GetVoidPointer(io_address), GetVoidPointer(buffer), readable_size),  svc::ResultInvalidPointer());
                        }
                        break;
                }
            }

            /* Advance. */
            buffer      += cur_size;
            cur_address += cur_size;
            remaining   -= cur_size;
        }

        return ResultSuccess();
    }

    Result KDebugBase::GetRunningThreadInfo(ams::svc::LastThreadContext *out_context, u64 *out_thread_id) {
        /* Get the attached process. */
        KScopedAutoObject process = this->GetProcess();
        R_UNLESS(process.IsNotNull(), svc::ResultProcessTerminated());

        /* Get the thread info. */
        {
            KScopedSchedulerLock sl;

            /* Get the running thread. */
            const s32 core_id = GetCurrentCoreId();
            KThread *thread = process->GetRunningThread(core_id);

            /* Check that the thread's idle count is correct. */
            R_UNLESS(process->GetRunningThreadIdleCount(core_id) == Kernel::GetScheduler(core_id).GetIdleCount(), svc::ResultNoThread());

            /* Check that the thread is running on the current core. */
            R_UNLESS(thread != nullptr,                  svc::ResultUnknownThread());
            R_UNLESS(thread->GetActiveCore() == core_id, svc::ResultUnknownThread());

            /* Get the thread's exception context. */
            GetExceptionContext(thread)->GetSvcThreadContext(out_context);

            /* Get the thread's id. */
            *out_thread_id = thread->GetId();
        }

        return ResultSuccess();
    }

    Result KDebugBase::Attach(KProcess *target) {
        /* Check that the process isn't null. */
        MESOSPHERE_ASSERT(target != nullptr);

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
                    case KProcess::State_Crashed:
                        break;
                    case KProcess::State_CreatedAttached:
                    case KProcess::State_RunningAttached:
                    case KProcess::State_DebugBreak:
                        return svc::ResultBusy();
                    case KProcess::State_Terminating:
                    case KProcess::State_Terminated:
                        return svc::ResultProcessTerminated();
                    MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
                }

                /* Set our process member, and open a reference to the target. */
                m_process = target;
                m_process->Open();

                /* Set ourselves as the process's attached object. */
                m_old_process_state = m_process->SetDebugObject(this);

                /* Send an event for our attaching to the process. */
                this->PushDebugEvent(ams::svc::DebugEvent_CreateProcess);

                /* Send events for attaching to each thread in the process. */
                {
                    auto end = m_process->GetThreadList().end();
                    for (auto it = m_process->GetThreadList().begin(); it != end; ++it) {
                        /* Request that we suspend the thread. */
                        it->RequestSuspend(KThread::SuspendType_Debug);

                        /* If the thread is in a state for us to do so, generate the event. */
                        if (const auto thread_state = it->GetState(); thread_state == KThread::ThreadState_Runnable || thread_state == KThread::ThreadState_Waiting) {
                            /* Mark the thread as attached to. */
                            it->SetDebugAttached();

                            /* Send the event. */
                            this->PushDebugEvent(ams::svc::DebugEvent_CreateThread, it->GetId(), GetInteger(it->GetThreadLocalRegionAddress()));
                        }
                    }
                }

                /* Send the process's jit debug info, if relevant. */
                if (KEventInfo *jit_info = m_process->GetJitDebugInfo(); jit_info != nullptr) {
                    this->EnqueueDebugEventInfo(jit_info);
                }

                /* Send an exception event to represent our attaching. */
                this->PushDebugEvent(ams::svc::DebugEvent_Exception, ams::svc::DebugException_DebuggerAttached);

                /* Signal. */
                this->NotifyAvailable();
            }
        }

        return ResultSuccess();
    }

    Result KDebugBase::BreakProcess() {
        /* Get the attached process. */
        KScopedAutoObject target = this->GetProcess();
        R_UNLESS(target.IsNotNull(), svc::ResultProcessTerminated());

        /* Lock both ourselves, the target process, and the scheduler. */
        KScopedLightLock state_lk(target->GetStateLock());
        KScopedLightLock list_lk(target->GetListLock());
        KScopedLightLock this_lk(m_lock);
        KScopedSchedulerLock sl;

        /* Check that we're still attached to the process, and that it's not terminated. */
        /* NOTE: Here Nintendo only checks that this->process is not nullptr. */
        R_UNLESS(m_process == target.GetPointerUnsafe(), svc::ResultProcessTerminated());
        R_UNLESS(!target->IsTerminated(),                    svc::ResultProcessTerminated());

        /* Get the currently active threads. */
        constexpr u64 ThreadIdNoThread      = -1ll;
        constexpr u64 ThreadIdUnknownThread = -2ll;
        u64 thread_ids[cpu::NumCores];
        for (size_t i = 0; i < util::size(thread_ids); ++i) {
            /* Get the currently running thread. */
            KThread *thread = target->GetRunningThread(i);

            /* Check that the thread's idle count is correct. */
            if (target->GetRunningThreadIdleCount(i) == Kernel::GetScheduler(i).GetIdleCount()) {
                if (thread != nullptr && static_cast<size_t>(thread->GetActiveCore()) == i) {
                    thread_ids[i] = thread->GetId();
                } else {
                    /* We found an unknown thread. */
                    thread_ids[i] = ThreadIdUnknownThread;
                }
            } else {
                /* We didn't find a thread. */
                thread_ids[i] = ThreadIdNoThread;
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
        static_assert(util::size(thread_ids) >= 4);
        this->PushDebugEvent(ams::svc::DebugEvent_Exception, ams::svc::DebugException_DebuggerBreak, thread_ids[0], thread_ids[1], thread_ids[2], thread_ids[3]);

        /* Signal. */
        this->NotifyAvailable();

        /* Set the process as breaked. */
        target->SetDebugBreak();

        return ResultSuccess();
    }

    Result KDebugBase::TerminateProcess() {
        /* Get the attached process. If we don't have one, we have nothing to do. */
        KScopedAutoObject target = this->GetProcess();
        R_SUCCEED_IF(target.IsNull());

        /* Detach from the process. */
        {
            /* Lock both ourselves and the target process. */
            KScopedLightLock state_lk(target->GetStateLock());
            KScopedLightLock list_lk(target->GetListLock());
            KScopedLightLock this_lk(m_lock);

            /* Check that we still have our process. */
            if (m_process != nullptr) {
                /* Check that our process is the one we got earlier. */
                MESOSPHERE_ASSERT(m_process == target.GetPointerUnsafe());

                /* Lock the scheduler. */
                KScopedSchedulerLock sl;

                /* Get the process's state. */
                const KProcess::State state = target->GetState();

                /* Check that the process is in a state where we can terminate it. */
                R_UNLESS(state != KProcess::State_Created,         svc::ResultInvalidState());
                R_UNLESS(state != KProcess::State_CreatedAttached, svc::ResultInvalidState());

                /* Decide on a new state for the process. */
                KProcess::State new_state;
                if (state == KProcess::State_RunningAttached) {
                    /* If the process is running, transition it accordingly. */
                    new_state = KProcess::State_Running;
                } else if (state == KProcess::State_DebugBreak) {
                    /* If the process is debug breaked, transition it accordingly. */
                    new_state = KProcess::State_Crashed;
                } else {
                    /* Otherwise, don't transition. */
                    new_state = state;
                }

                /* Detach from the process. */
                target->ClearDebugObject(new_state);
                m_process = nullptr;

                /* Clear our continue flags. */
                m_continue_flags = 0;
            }
        }

        /* Close the reference we held to the process while we were attached to it. */
        target->Close();

        /* Terminate the process. */
        target->Terminate();

        return ResultSuccess();
    }

    Result KDebugBase::GetThreadContext(ams::svc::ThreadContext *out, u64 thread_id, u32 context_flags) {
        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Get the thread from its id. */
        KThread *thread = KThread::GetThreadFromId(thread_id);
        R_UNLESS(thread != nullptr, svc::ResultInvalidId());
        ON_SCOPE_EXIT { thread->Close(); };

        /* Verify that the thread is owned by our process. */
        R_UNLESS(m_process == thread->GetOwnerProcess(), svc::ResultInvalidId());

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
                    }
                    break;
                }

                /* If the thread is current, retry until it isn't. */
                if (current) {
                    continue;
                }
            }

            /* Get the thread context. */
            return this->GetThreadContextImpl(out, thread, context_flags);
        }
    }

    Result KDebugBase::SetThreadContext(const ams::svc::ThreadContext &ctx, u64 thread_id, u32 context_flags) {
        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Get the thread from its id. */
        KThread *thread = KThread::GetThreadFromId(thread_id);
        R_UNLESS(thread != nullptr, svc::ResultInvalidId());
        ON_SCOPE_EXIT { thread->Close(); };

        /* Verify that the thread is owned by our process. */
        R_UNLESS(m_process == thread->GetOwnerProcess(), svc::ResultInvalidId());

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
                    }
                    break;
                }

                /* If the thread is current, retry until it isn't. */
                if (current) {
                    continue;
                }
            }

            /* Verify that the thread's svc state is valid. */
            if (thread->IsCallingSvc()) {
                R_UNLESS(thread->GetSvcId() != svc::SvcId_Break,               svc::ResultInvalidState());
                R_UNLESS(thread->GetSvcId() != svc::SvcId_ReturnFromException, svc::ResultInvalidState());
            }

            /* Set the thread context. */
            return this->SetThreadContextImpl(ctx, thread, context_flags);
        }
    }


    Result KDebugBase::ContinueDebug(const u32 flags, const u64 *thread_ids, size_t num_thread_ids) {
        /* Get the attached process. */
        KScopedAutoObject target = this->GetProcess();
        R_UNLESS(target.IsNotNull(), svc::ResultProcessTerminated());

        /* Lock both ourselves, the target process, and the scheduler. */
        KScopedLightLock state_lk(target->GetStateLock());
        KScopedLightLock list_lk(target->GetListLock());
        KScopedLightLock this_lk(m_lock);
        KScopedSchedulerLock sl;

        /* Check that we're still attached to the process, and that it's not terminated. */
        R_UNLESS(m_process == target.GetPointerUnsafe(), svc::ResultProcessTerminated());
        R_UNLESS(!target->IsTerminated(),                    svc::ResultProcessTerminated());

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

        return ResultSuccess();
    }

    KEventInfo *KDebugBase::CreateDebugEvent(ams::svc::DebugEvent event, uintptr_t param0, uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4, u64 cur_thread_id) {
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
                        /* ... */
                    }
                    break;
                case ams::svc::DebugEvent_CreateThread:
                    {
                        /* Set the thread id. */
                        info->thread_id = param0;

                        /* Set the thread creation info. */
                        info->info.create_thread.thread_id   = param0;
                        info->info.create_thread.tls_address = param1;
                    }
                    break;
                case ams::svc::DebugEvent_ExitProcess:
                    {
                        /* Set the exit reason. */
                        info->info.exit_process.reason = static_cast<ams::svc::ProcessExitReason>(param0);

                        /* Clear the thread id and flags. */
                        info->thread_id = 0;
                        info->flags = 0;
                    }
                    break;
                case ams::svc::DebugEvent_ExitThread:
                    {
                        /* Set the thread id. */
                        info->thread_id = param0;

                        /* Set the exit reason. */
                        info->info.exit_thread.reason = static_cast<ams::svc::ThreadExitReason>(param1);
                    }
                    break;
                case ams::svc::DebugEvent_Exception:
                    {
                        /* Set the thread id. */
                        info->thread_id = cur_thread_id;

                        /* Set the exception type, and clear the count. */
                        info->info.exception.exception_type       = static_cast<ams::svc::DebugException>(param0);
                        info->info.exception.exception_data_count = 0;
                        switch (static_cast<ams::svc::DebugException>(param0)) {
                            case ams::svc::DebugException_UndefinedInstruction:
                            case ams::svc::DebugException_BreakPoint:
                            case ams::svc::DebugException_UndefinedSystemCall:
                                {
                                    info->info.exception.exception_address    = param1;

                                    info->info.exception.exception_data_count = 1;
                                    info->info.exception.exception_data[0]    = param2;
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
                                    info->info.exception.exception_address    = param1;

                                    info->info.exception.exception_data_count = 3;
                                    info->info.exception.exception_data[0]    = param2;
                                    info->info.exception.exception_data[1]    = param3;
                                    info->info.exception.exception_data[2]    = param4;
                                }
                                break;
                            case ams::svc::DebugException_DebuggerBreak:
                                {
                                    info->thread_id = 0;

                                    info->info.exception.exception_address    = 0;

                                    info->info.exception.exception_data_count = 4;
                                    info->info.exception.exception_data[0]    = param1;
                                    info->info.exception.exception_data[1]    = param2;
                                    info->info.exception.exception_data[2]    = param3;
                                    info->info.exception.exception_data[3]    = param4;
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
                                    info->info.exception.exception_address = param1;
                                }
                                break;
                        }
                    }
                    break;
            }
        }

        return info;
    }

    void KDebugBase::PushDebugEvent(ams::svc::DebugEvent event, uintptr_t param0, uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4) {
        /* Create and enqueue and event. */
        if (KEventInfo *new_info = CreateDebugEvent(event, param0, param1, param2, param3, param4, GetCurrentThread().GetId()); new_info != nullptr) {
            this->EnqueueDebugEventInfo(new_info);
        }
    }

    void KDebugBase::EnqueueDebugEventInfo(KEventInfo *info) {
        /* Lock the scheduler. */
        KScopedSchedulerLock sl;

        /* Push the event to the back of the list. */
        m_event_info_list.push_back(*info);
    }


    KScopedAutoObject<KProcess> KDebugBase::GetProcess() {
        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        return m_process;
    }

    template<typename T> requires (std::same_as<T, ams::svc::lp64::DebugEventInfo> || std::same_as<T, ams::svc::ilp32::DebugEventInfo>)
    Result KDebugBase::GetDebugEventInfoImpl(T *out) {
        /* Get the attached process. */
        KScopedAutoObject process = this->GetProcess();
        R_UNLESS(process.IsNotNull(), svc::ResultProcessTerminated());

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
                                out->info.exception.specific.undefined_instruction.insn = info->info.exception.exception_data[0];
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
                                MESOSPHERE_ASSERT(info->info.exception.exception_data_count == 4);
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

        return ResultSuccess();
    }

    Result KDebugBase::GetDebugEventInfo(ams::svc::lp64::DebugEventInfo *out) {
        return this->GetDebugEventInfoImpl(out);
    }

    Result KDebugBase::GetDebugEventInfo(ams::svc::ilp32::DebugEventInfo *out) {
        return this->GetDebugEventInfoImpl(out);
    }

    void KDebugBase::OnFinalizeSynchronizationObject() {
        /* Detach from our process, if we have one. */
        {
            /* Get the attached process. */
            KScopedAutoObject process = this->GetProcess();

            /* If the process isn't null, detach. */
            if (process.IsNotNull()) {
                /* When we're done detaching, clear the reference we opened when we attached. */
                ON_SCOPE_EXIT { process->Close(); };

                /* Detach. */
                {
                    /* Lock both ourselves and the target process. */
                    KScopedLightLock state_lk(process->GetStateLock());
                    KScopedLightLock list_lk(process->GetListLock());
                    KScopedLightLock this_lk(m_lock);

                    /* Ensure we finalize exactly once. */
                    if (m_process != nullptr) {
                        MESOSPHERE_ASSERT(m_process == process.GetPointerUnsafe());
                        {
                            KScopedSchedulerLock sl;

                            /* Detach ourselves from the process. */
                            process->ClearDebugObject(m_old_process_state);

                            /* Release all threads. */
                            const bool resume = (process->GetState() != KProcess::State_Crashed);
                            {
                                auto end = process->GetThreadList().end();
                                for (auto it = process->GetThreadList().begin(); it != end; ++it) {
                                    if (resume) {
                                        /* If the process isn't crashed, resume threads. */
                                        it->Resume(KThread::SuspendType_Debug);
                                    } else {
                                        /* Otherwise, suspend them. */
                                        it->RequestSuspend(KThread::SuspendType_Debug);
                                    }
                                }
                            }

                            /* Clear our process. */
                            m_process = nullptr;
                        }
                    }
                }
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
        KScopedSchedulerLock sl;

        return (!m_event_info_list.empty()) || m_process == nullptr || m_process->IsTerminated();
    }

    Result KDebugBase::ProcessDebugEvent(ams::svc::DebugEvent event, uintptr_t param0, uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4) {
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
                return svc::ResultNotHandled();
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
            debug->PushDebugEvent(event, param0, param1, param2, param3, param4);
            debug->NotifyAvailable();

            /* Set the process as breaked. */
            process->SetDebugBreak();

            /* If the event is an exception, set the result. */
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
                return GetCurrentThread().GetDebugExceptionResult();
            } else {
                /* We don't have a debug object, so stop processing the exception. */
                return svc::ResultStopProcessingException();
            }
        }

        return ResultSuccess();
    }

    Result KDebugBase::OnDebugEvent(ams::svc::DebugEvent event, uintptr_t param0, uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4) {
        if (KProcess *process = GetCurrentProcessPointer(); process != nullptr && process->IsAttachedToDebugger()) {
            return ProcessDebugEvent(event, param0, param1, param2, param3, param4);
        }
        return ResultSuccess();
    }

    Result KDebugBase::OnExitProcess(KProcess *process) {
        MESOSPHERE_ASSERT(process != nullptr);

        /* Check if we're attached to a debugger. */
        if (process->IsAttachedToDebugger()) {
            /* If we are, lock the scheduler. */
            KScopedSchedulerLock sl;

            /* Push the event. */
            if (KDebugBase *debug = GetDebugObject(process); debug != nullptr) {
                debug->PushDebugEvent(ams::svc::DebugEvent_ExitProcess, ams::svc::ProcessExitReason_ExitProcess);
                debug->NotifyAvailable();
            }
        }

        return ResultSuccess();
    }

    Result KDebugBase::OnTerminateProcess(KProcess *process) {
        MESOSPHERE_ASSERT(process != nullptr);

        /* Check if we're attached to a debugger. */
        if (process->IsAttachedToDebugger()) {
            /* If we are, lock the scheduler. */
            KScopedSchedulerLock sl;

            /* Push the event. */
            if (KDebugBase *debug = GetDebugObject(process); debug != nullptr) {
                debug->PushDebugEvent(ams::svc::DebugEvent_ExitProcess, ams::svc::ProcessExitReason_TerminateProcess);
                debug->NotifyAvailable();
            }
        }

        return ResultSuccess();
    }

    Result KDebugBase::OnExitThread(KThread *thread) {
        MESOSPHERE_ASSERT(thread != nullptr);

        /* Check if we're attached to a debugger. */
        if (KProcess *process = thread->GetOwnerProcess(); process != nullptr && process->IsAttachedToDebugger()) {
            /* If we are, submit the event. */
            R_TRY(OnDebugEvent(ams::svc::DebugEvent_ExitThread, thread->GetId(), thread->IsTerminationRequested() ? ams::svc::ThreadExitReason_TerminateThread : ams::svc::ThreadExitReason_ExitThread));
        }

        return ResultSuccess();
    }

}
