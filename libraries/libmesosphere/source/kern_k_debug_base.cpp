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
        this->process        = nullptr;
        this->continue_flags = 0;
    }

    Result KDebugBase::Attach(KProcess *target) {
        /* Check that the process isn't null. */
        MESOSPHERE_ASSERT(target != nullptr);

        /* Attach to the process. */
        {
            /* Lock both ourselves, the target process, and the scheduler. */
            KScopedLightLock state_lk(target->GetStateLock());
            KScopedLightLock list_lk(target->GetListLock());
            KScopedLightLock this_lk(this->lock);
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
                this->process = target;
                this->process->Open();

                /* Set ourselves as the process's attached object. */
                this->old_process_state = this->process->SetDebugObject(this);

                /* Send an event for our attaching to the process. */
                this->PushDebugEvent(ams::svc::DebugEvent_AttachProcess);

                /* Send events for attaching to each thread in the process. */
                {
                    auto end = this->process->GetThreadList().end();
                    for (auto it = this->process->GetThreadList().begin(); it != end; ++it) {
                        /* Request that we suspend the thread. */
                        it->RequestSuspend(KThread::SuspendType_Debug);

                        /* If the thread is in a state for us to do so, generate the event. */
                        if (const auto thread_state = it->GetState(); thread_state == KThread::ThreadState_Runnable || thread_state == KThread::ThreadState_Waiting) {
                            /* Mark the thread as attached to. */
                            it->SetDebugAttached();

                            /* Send the event. */
                            this->PushDebugEvent(ams::svc::DebugEvent_AttachThread, it->GetId(), GetInteger(it->GetThreadLocalRegionAddress()), it->GetEntrypoint());
                        }
                    }
                }

                /* Send the process's jit debug info, if relevant. */
                if (KEventInfo *jit_info = this->process->GetJitDebugInfo(); jit_info != nullptr) {
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

    KEventInfo *KDebugBase::CreateDebugEvent(ams::svc::DebugEvent event, uintptr_t param0, uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4, u64 cur_thread_id) {
        /* Allocate a new event. */
        KEventInfo *info = KEventInfo::Allocate();

        /* Populate the event info. */
        if (info != nullptr) {
            /* Set common fields. */
            info->event     = event;
            info->thread_id = 0;
            info->flags     = 1; /* TODO: enum this in ams::svc */

            /* Set event specific fields. */
            switch (event) {
                case ams::svc::DebugEvent_AttachProcess:
                    {
                        /* ... */
                    }
                    break;
                case ams::svc::DebugEvent_AttachThread:
                    {
                        /* Set the thread id. */
                        info->thread_id = param0;

                        /* Set the thread creation info. */
                        info->info.create_thread.thread_id   = param0;
                        info->info.create_thread.tls_address = param1;
                        info->info.create_thread.entrypoint  = param2;
                    }
                    break;
                case ams::svc::DebugEvent_ExitProcess:
                    {
                        /* Set the exit reason. */
                        info->info.exit_process.reason = static_cast<ams::svc::ProcessExitReason>(param0);

                        /* Clear the thread id and flags. */
                        info->thread_id = 0;
                        info->flags = 0 /* TODO: enum this in ams::svc */;
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
        this->event_info_list.push_back(*info);
    }


    KScopedAutoObject<KProcess> KDebugBase::GetProcess() {
        /* Lock ourselves. */
        KScopedLightLock lk(this->lock);

        return this->process;
    }

    Result KDebugBase::GetDebugEventInfo(ams::svc::lp64::DebugEventInfo *out) {
        /* Get the attached process. */
        KScopedAutoObject process = this->GetProcess();
        R_UNLESS(process.IsNotNull(), svc::ResultProcessTerminated());

        /* Pop an event info from our queue. */
        KEventInfo *info = nullptr;
        {
            KScopedSchedulerLock sl;

            /* Check that we have an event to dequeue. */
            R_UNLESS(!this->event_info_list.empty(), svc::ResultNoEvent());

            /* Pop the event from the front of the queue. */
            info = std::addressof(this->event_info_list.front());
            this->event_info_list.pop_front();
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
            case ams::svc::DebugEvent_AttachProcess:
                {
                    out->info.attach_process.program_id                     = process->GetProgramId();
                    out->info.attach_process.process_id                     = process->GetId();
                    out->info.attach_process.flags                          = process->GetCreateProcessFlags();
                    out->info.attach_process.user_exception_context_address = GetInteger(process->GetProcessLocalRegionAddress());

                    std::memcpy(out->info.attach_process.name, process->GetName(), sizeof(out->info.attach_process.name));
                }
                break;
            case ams::svc::DebugEvent_AttachThread:
                {
                    out->info.attach_thread.thread_id   = info->info.create_thread.thread_id;
                    out->info.attach_thread.tls_address = info->info.create_thread.tls_address;
                    out->info.attach_thread.entrypoint  = info->info.create_thread.entrypoint;
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

    Result KDebugBase::GetDebugEventInfo(ams::svc::ilp32::DebugEventInfo *out) {
        MESOSPHERE_UNIMPLEMENTED();
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
                    KScopedLightLock this_lk(this->lock);

                    /* Ensure we finalize exactly once. */
                    if (this->process != nullptr) {
                        MESOSPHERE_ASSERT(this->process == process.GetPointerUnsafe());
                        {
                            KScopedSchedulerLock sl;

                            /* Detach ourselves from the process. */
                            process->ClearDebugObject(this->old_process_state);

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
                            this->process = nullptr;
                        }
                    }
                }
            }
        }

        /* Free any pending events. */
        {
            KScopedSchedulerLock sl;

            while (!this->event_info_list.empty()) {
                KEventInfo *info = std::addressof(this->event_info_list.front());
                this->event_info_list.pop_front();
                KEventInfo::Free(info);
            }
        }
    }

    bool KDebugBase::IsSignaled() const {
        KScopedSchedulerLock sl;

        return (!this->event_info_list.empty()) || this->process == nullptr || this->process->IsTerminated();
    }

    Result KDebugBase::ProcessDebugEvent(ams::svc::DebugEvent event, uintptr_t param0, uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4) {
        MESOSPHERE_UNIMPLEMENTED();
    }

    Result KDebugBase::OnDebugEvent(ams::svc::DebugEvent event, uintptr_t param0, uintptr_t param1, uintptr_t param2, uintptr_t param3, uintptr_t param4) {
        if (KProcess *process = GetCurrentProcessPointer(); process != nullptr && process->IsAttachedToDebugger()) {
            return ProcessDebugEvent(event, param0, param1, param2, param3, param4);
        }
        return ResultSuccess();
    }

    Result KDebugBase::OnExitProcess(KProcess *process) {
        MESOSPHERE_ASSERT(process != nullptr);

        if (process->IsAttachedToDebugger()) {
            KScopedSchedulerLock sl;

            if (KDebugBase *debug = GetDebugObject(process); debug != nullptr) {
                debug->PushDebugEvent(ams::svc::DebugEvent_ExitProcess, ams::svc::ProcessExitReason_ExitProcess);
                debug->NotifyAvailable();
            }
        }

        return ResultSuccess();
    }

    Result KDebugBase::OnTerminateProcess(KProcess *process) {
        MESOSPHERE_ASSERT(process != nullptr);

        if (process->IsAttachedToDebugger()) {
            KScopedSchedulerLock sl;

            if (KDebugBase *debug = GetDebugObject(process); debug != nullptr) {
                debug->PushDebugEvent(ams::svc::DebugEvent_ExitProcess, ams::svc::ProcessExitReason_TerminateProcess);
                debug->NotifyAvailable();
            }
        }

        return ResultSuccess();
    }

    Result KDebugBase::OnExitThread(KThread *thread) {
        MESOSPHERE_ASSERT(thread != nullptr);

        if (KProcess *process = thread->GetOwnerProcess(); process != nullptr && process->IsAttachedToDebugger()) {
            R_TRY(OnDebugEvent(ams::svc::DebugEvent_ExitThread, thread->GetId(), thread->IsTerminationRequested() ? ams::svc::ThreadExitReason_TerminateThread : ams::svc::ThreadExitReason_ExitThread));
        }

        return ResultSuccess();
    }

}
