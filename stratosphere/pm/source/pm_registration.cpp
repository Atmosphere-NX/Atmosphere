/*
 * Copyright (c) 2018 Atmosphère-NX
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
 
#include <switch.h>
#include <stratosphere.hpp>
#include <atomic>

#include "pm_registration.hpp"
#include "pm_resource_limits.hpp"
#include "pm_process_wait.hpp"

static ProcessList g_process_list;
static ProcessList g_dead_process_list;

static IEvent *g_process_launch_start_event = nullptr;
static HosSemaphore g_sema_finish_launch;

static HosMutex g_process_launch_mutex;
static Registration::ProcessLaunchState g_process_launch_state;

static std::atomic_bool g_debug_next_application(false);
static std::atomic<u64> g_debug_on_launch_tid(0);

static IEvent *g_process_event = nullptr;
static IEvent *g_debug_title_event = nullptr;
static IEvent *g_debug_application_event = nullptr;

std::unique_lock<HosRecursiveMutex> Registration::GetProcessListUniqueLock() {
    return g_process_list.GetUniqueLock();
}

void Registration::InitializeSystemResources() {
    g_process_event = CreateWriteOnlySystemEvent();
    g_debug_title_event = CreateWriteOnlySystemEvent();
    g_debug_application_event = CreateWriteOnlySystemEvent();
    
    /* Auto-clear non-system event. */
    g_process_launch_start_event = CreateHosEvent(&Registration::ProcessLaunchStartCallback);
    
    ResourceLimitUtils::InitializeLimits();
}

Result Registration::ProcessLaunchStartCallback(u64 timeout) {
    g_process_launch_start_event->Clear();
    Registration::HandleProcessLaunch();
    return 0;
}

IWaitable *Registration::GetProcessLaunchStartEvent() {
    return g_process_launch_start_event;
}

void Registration::HandleProcessLaunch() {
    LoaderProgramInfo program_info = {0};
    Result rc;
    u64 launch_flags = g_process_launch_state.launch_flags;
    u64 *out_pid = g_process_launch_state.out_pid;
    Process new_process = {0};
    new_process.tid_sid = g_process_launch_state.tid_sid;
    auto ac_buf = std::vector<u8>(4 * sizeof(LoaderProgramInfo));
    std::fill(ac_buf.begin(), ac_buf.end(), 0xCC);
    u8 *acid_sac = ac_buf.data(), *aci0_sac = acid_sac + sizeof(LoaderProgramInfo), *fac = aci0_sac + sizeof(LoaderProgramInfo), *fah = fac + sizeof(LoaderProgramInfo);
            
    /* Check that this is a real program. */
    if (R_FAILED((rc = ldrPmGetProgramInfo(new_process.tid_sid.title_id, new_process.tid_sid.storage_id, &program_info)))) {
        goto HANDLE_PROCESS_LAUNCH_END;
    }
        
    /* Get the resource limit handle, ensure that we can launch the program. */
    if ((program_info.application_type & 3) == 1 && HasApplicationProcess(NULL)) {
        rc = 0xA0F;
        goto HANDLE_PROCESS_LAUNCH_END;
    }
    
    /* Try to register the title for launch in loader... */
    if (R_FAILED((rc = ldrPmRegisterTitle(new_process.tid_sid.title_id, new_process.tid_sid.storage_id, &new_process.ldr_queue_index)))) {
        goto HANDLE_PROCESS_LAUNCH_END;
    }
        
    /* Make sure the previous application is cleaned up. */
    if ((program_info.application_type & 3) == 1) {
        ResourceLimitUtils::EnsureApplicationResourcesAvailable();
    }
        
    /* Try to create the process... */
    if (R_FAILED((rc = ldrPmCreateProcess(LAUNCHFLAGS_ARGLOW(launch_flags) | LAUNCHFLAGS_ARGHIGH(launch_flags), new_process.ldr_queue_index, ResourceLimitUtils::GetResourceLimitHandle(program_info.application_type), &new_process.handle)))) {
        goto PROCESS_CREATION_FAILED;
    }
        
    /* Get the new process's id. */
    svcGetProcessId(&new_process.pid, new_process.handle);
        
    /* Register with FS. */
    memcpy(fac, program_info.ac_buffer + program_info.acid_sac_size + program_info.aci0_sac_size, program_info.acid_fac_size);
    memcpy(fah, program_info.ac_buffer + program_info.acid_sac_size + program_info.aci0_sac_size + program_info.acid_fac_size, program_info.aci0_fah_size);
    if (R_FAILED((rc = fsprRegisterProgram(new_process.pid, new_process.tid_sid.title_id, new_process.tid_sid.storage_id, fah, program_info.aci0_fah_size, fac, program_info.acid_fac_size)))) {
        goto FS_REGISTRATION_FAILED;
    }
        
    /* Register with SM. */
    memcpy(acid_sac, program_info.ac_buffer, program_info.acid_sac_size);
    memcpy(aci0_sac, program_info.ac_buffer + program_info.acid_sac_size, program_info.aci0_sac_size);
    if (R_FAILED((rc = smManagerRegisterProcess(new_process.pid, acid_sac, program_info.acid_sac_size, aci0_sac, program_info.aci0_sac_size)))) {
        goto SM_REGISTRATION_FAILED;
    }
        
    /* Setup process flags. */
    if (program_info.application_type & 1) {
        new_process.flags |= PROCESSFLAGS_APPLICATION;
    }
    if (kernelAbove200() && LAUNCHFLAGS_NOTIYDEBUGSPECIAL(launch_flags) && (program_info.application_type & 4)) { 
        new_process.flags |= PROCESSFLAGS_NOTIFYDEBUGSPECIAL;
    }
    if (LAUNCHFLAGS_NOTIFYWHENEXITED(launch_flags)) {
        new_process.flags |= PROCESSFLAGS_NOTIFYWHENEXITED;
    }
    if (LAUNCHFLAGS_NOTIFYDEBUGEVENTS(launch_flags) && (!kernelAbove200() || (program_info.application_type & 4))) {
        new_process.flags |= PROCESSFLAGS_NOTIFYDEBUGEVENTS;
    }
    
    /* Add process to the list. */
    Registration::AddProcessToList(std::make_shared<Registration::Process>(new_process));
    
    /* Signal, if relevant. */
    if (new_process.tid_sid.title_id == g_debug_on_launch_tid.load()) {
        g_debug_title_event->Signal();
        g_debug_on_launch_tid = 0;
        rc = 0;
    } else if ((new_process.flags & PROCESSFLAGS_APPLICATION) && g_debug_next_application.load()) {
        g_debug_application_event->Signal();
        g_debug_next_application = false;
        rc = 0;
    } else if (LAUNCHFLAGS_STARTSUSPENDED(launch_flags)) {
        rc = 0;
    } else {
        rc = svcStartProcess(new_process.handle, program_info.main_thread_priority, program_info.default_cpu_id, program_info.main_thread_stack_size);
    
        if (R_SUCCEEDED(rc)) {
            SetProcessState(new_process.pid, ProcessState_Running);
        }
    }
    
    if (R_FAILED(rc)) {
        Registration::RemoveProcessFromList(new_process.pid);
        smManagerUnregisterProcess(new_process.pid);
    }
    
SM_REGISTRATION_FAILED:
    if (R_FAILED(rc)) {
        fsprUnregisterProgram(new_process.pid);
    }
    
FS_REGISTRATION_FAILED:
    if (R_FAILED(rc)) {
        svcCloseHandle(new_process.handle);
        new_process.handle = 0;
    }
    
PROCESS_CREATION_FAILED:
    if (R_FAILED(rc)) {
        ldrPmUnregisterTitle(new_process.ldr_queue_index);
    }
    
HANDLE_PROCESS_LAUNCH_END:
    g_process_launch_state.result = rc;
    if (R_SUCCEEDED(rc)) {
        *out_pid = new_process.pid;
    }
    
    g_sema_finish_launch.Signal();
}


Result Registration::LaunchDebugProcess(u64 pid) {
    auto auto_lock = GetProcessListUniqueLock();
    LoaderProgramInfo program_info = {0};
    Result rc;
    
    std::shared_ptr<Registration::Process> proc = GetProcess(pid);
    if (proc == NULL) {
        return 0x20F;
    }
    
    if (proc->state >= ProcessState_Running) {
        return 0x40F;
    }
    
    /* Check that this is a real program. */
    if (R_FAILED((rc = ldrPmGetProgramInfo(proc->tid_sid.title_id, proc->tid_sid.storage_id, &program_info)))) {
        return rc;
    }
    
    if (R_SUCCEEDED((rc = svcStartProcess(proc->handle, program_info.main_thread_priority, program_info.default_cpu_id, program_info.main_thread_stack_size)))) {
        proc->state = ProcessState_Running;
    }
    
    return rc;
}

Result Registration::LaunchProcess(u64 title_id, FsStorageId storage_id, u64 launch_flags, u64 *out_pid) {
    /* Only allow one mutex to exist. */
    std::scoped_lock lk{g_process_launch_mutex};
    g_process_launch_state.tid_sid.title_id = title_id;
    g_process_launch_state.tid_sid.storage_id = storage_id;
    g_process_launch_state.launch_flags = launch_flags;
    g_process_launch_state.out_pid = out_pid;
    
    /* Start a launch, and wait for it to exit. */
    g_process_launch_start_event->Signal();
    g_sema_finish_launch.Wait();
        
    return g_process_launch_state.result;
}

Result Registration::LaunchProcessByTidSid(TidSid tid_sid, u64 launch_flags, u64 *out_pid) {
    return LaunchProcess(tid_sid.title_id, tid_sid.storage_id, launch_flags, out_pid);
};

Result Registration::HandleSignaledProcess(std::shared_ptr<Registration::Process> process) {
    u64 tmp;
     
    /* Reset the signal. */
    svcResetSignal(process->handle);
    
    ProcessState old_state;
    old_state = process->state;
    svcGetProcessInfo(&tmp, process->handle, ProcessInfoType_ProcessState);
    process->state = (ProcessState)tmp;
    
    if (old_state == ProcessState_Crashed && process->state != ProcessState_Crashed) {
        process->flags &= ~PROCESSFLAGS_CRASH_DEBUG;
    }
    switch (process->state) {
        case ProcessState_Created:
        case ProcessState_CreatedAttached:
        case ProcessState_Exiting:
            break;
        case ProcessState_Running:
            if (process->flags & PROCESSFLAGS_NOTIFYDEBUGEVENTS) {
                process->flags &= ~(PROCESSFLAGS_DEBUGEVENTPENDING | PROCESSFLAGS_DEBUGSUSPENDED);
                process->flags |= PROCESSFLAGS_DEBUGEVENTPENDING;
                g_process_event->Signal();
            }
            if (kernelAbove200() && process->flags & PROCESSFLAGS_NOTIFYDEBUGSPECIAL) {
                process->flags &= ~(PROCESSFLAGS_NOTIFYDEBUGSPECIAL | PROCESSFLAGS_DEBUGDETACHED);
                process->flags |= PROCESSFLAGS_DEBUGDETACHED;
            }
            break;
        case ProcessState_Crashed:
            process->flags |= (PROCESSFLAGS_CRASHED | PROCESSFLAGS_CRASH_DEBUG);
            g_process_event->Signal();
            break;
        case ProcessState_RunningAttached:
            if (process->flags & PROCESSFLAGS_NOTIFYDEBUGEVENTS) {
                process->flags &= ~(PROCESSFLAGS_DEBUGEVENTPENDING | PROCESSFLAGS_DEBUGSUSPENDED);
                process->flags |= PROCESSFLAGS_DEBUGEVENTPENDING;
                g_process_event->Signal();
            }
            break;
        case ProcessState_Exited:
            if (process->flags & PROCESSFLAGS_NOTIFYWHENEXITED && !kernelAbove500()) {
                g_process_event->Signal();
            } else {
                FinalizeExitedProcess(process);
            }
            return 0xF601;
        case ProcessState_DebugSuspended:
            if (process->flags & PROCESSFLAGS_NOTIFYDEBUGEVENTS) {
                process->flags |= (PROCESSFLAGS_DEBUGEVENTPENDING | PROCESSFLAGS_DEBUGSUSPENDED);
                g_process_event->Signal();
            }
            break;
    }
    return 0;
}

void Registration::FinalizeExitedProcess(std::shared_ptr<Registration::Process> process) {
    auto auto_lock = GetProcessListUniqueLock();
    bool signal_debug_process_5x = kernelAbove500() && process->flags & PROCESSFLAGS_NOTIFYWHENEXITED;
    /* Unregister with FS. */
    if (R_FAILED(fsprUnregisterProgram(process->pid))) {
        std::abort();
    }
    /* Unregister with SM. */
    if (R_FAILED(smManagerUnregisterProcess(process->pid))) {
        std::abort();
    }
    /* Unregister with LDR. */
    if (R_FAILED(ldrPmUnregisterTitle(process->ldr_queue_index))) {
        std::abort();
    }
    
    /* Close the process's handle. */
    svcCloseHandle(process->handle);
    process->handle = 0;
    
    /* Insert into dead process list, if relevant. */
    if (signal_debug_process_5x) {
        auto lk = g_dead_process_list.GetUniqueLock();
        g_dead_process_list.processes.push_back(process);
    }
    
    /* Remove NOTE: This probably frees process. */
    RemoveProcessFromList(process->pid);

    auto_lock.unlock();
    if (signal_debug_process_5x) {
        g_process_event->Signal();
    }
}

void Registration::AddProcessToList(std::shared_ptr<Registration::Process> process) {
    auto auto_lock = GetProcessListUniqueLock();
    g_process_list.processes.push_back(process);
    g_process_launch_start_event->GetManager()->AddWaitable(new ProcessWaiter(process));
}

void Registration::RemoveProcessFromList(u64 pid) {
    auto auto_lock = GetProcessListUniqueLock();
    
    /* Remove process from list. */
    for (unsigned int i = 0; i < g_process_list.processes.size(); i++) {
        std::shared_ptr<Registration::Process> process = g_process_list.processes[i];
        if (process->pid == pid) {
            g_process_list.processes.erase(g_process_list.processes.begin() + i);
            svcCloseHandle(process->handle);
            process->handle = 0;
            break;
        }
    }
}

void Registration::SetProcessState(u64 pid, ProcessState new_state) {
    auto auto_lock = GetProcessListUniqueLock();
    
    /* Set process state. */
    for (auto &process : g_process_list.processes) {
        if (process->pid == pid) {      
            process->state = new_state;
            break;
        }
    }
}

bool Registration::HasApplicationProcess(std::shared_ptr<Registration::Process> *out) {
    auto auto_lock = GetProcessListUniqueLock();
    
    for (auto &process : g_process_list.processes) {
        if (process->flags & PROCESSFLAGS_APPLICATION) {
            if (out != nullptr) {
                *out = process;
            }
            return true;
        }
    }
    
    return false;
}

std::shared_ptr<Registration::Process> Registration::GetProcess(u64 pid) {
    auto auto_lock = GetProcessListUniqueLock();
    
    for (auto &process : g_process_list.processes) {
        if (process->pid == pid) {
            return process;
        }
    }
    
    return nullptr;
}

std::shared_ptr<Registration::Process> Registration::GetProcessByTitleId(u64 tid) {
    auto auto_lock = GetProcessListUniqueLock();
    
    for (auto &process : g_process_list.processes) {
        if (process->tid_sid.title_id == tid) {
            return process;
        }
    }
    
    return nullptr;
}


Result Registration::GetDebugProcessIds(u64 *out_pids, u32 max_out, u32 *num_out) {
    auto auto_lock = GetProcessListUniqueLock();
    u32 num = 0;
    

    for (auto &process : g_process_list.processes) {
        if (process->flags & PROCESSFLAGS_CRASH_DEBUG && num < max_out) {
            out_pids[num++] = process->pid;
        }
    }
    
    *num_out = num;
    return 0;
}

Handle Registration::GetProcessEventHandle() {
    return g_process_event->GetHandle();
}

void Registration::GetProcessEventType(u64 *out_pid, u64 *out_type) {
    auto auto_lock = GetProcessListUniqueLock();
    
    for (auto &p : g_process_list.processes) {
        if (kernelAbove200() && p->state >= ProcessState_Running && p->flags & PROCESSFLAGS_DEBUGDETACHED) {
            p->flags &= ~PROCESSFLAGS_DEBUGDETACHED;
            *out_pid = p->pid;
            *out_type = kernelAbove500() ? PROCESSEVENTTYPE_500_DEBUGDETACHED : PROCESSEVENTTYPE_DEBUGDETACHED;
            return;
        }
        if (p->flags & PROCESSFLAGS_DEBUGEVENTPENDING) {
            u64 old_flags = p->flags;
            p->flags &= ~PROCESSFLAGS_DEBUGEVENTPENDING;
            *out_pid = p->pid;
            *out_type = kernelAbove500() ?
                ((old_flags & PROCESSFLAGS_DEBUGSUSPENDED) ?
                    PROCESSEVENTTYPE_500_SUSPENDED :
                    PROCESSEVENTTYPE_500_RUNNING) :
                ((old_flags & PROCESSFLAGS_DEBUGSUSPENDED) ?
                    PROCESSEVENTTYPE_SUSPENDED :
                    PROCESSEVENTTYPE_RUNNING);
            return;
        }
        if (p->flags & PROCESSFLAGS_CRASHED) {
            *out_pid = p->pid;
            *out_type = kernelAbove500() ? PROCESSEVENTTYPE_500_CRASH : PROCESSEVENTTYPE_CRASH;
            return;
        }
        if (!kernelAbove500() && p->flags & PROCESSFLAGS_NOTIFYWHENEXITED && p->state == ProcessState_Exited) {
            *out_pid = p->pid;
            *out_type = PROCESSEVENTTYPE_EXIT;
            return;
        }
    }
    if (kernelAbove500()) {
        auto_lock.unlock();
        auto dead_process_list_lock = g_dead_process_list.GetUniqueLock();
        if (g_dead_process_list.processes.size()) {
            std::shared_ptr<Registration::Process> process = g_dead_process_list.processes[0];
            g_dead_process_list.processes.erase(g_dead_process_list.processes.begin());
            *out_pid = process->pid;
            *out_type = PROCESSEVENTTYPE_500_EXIT;
            return;
        }
    }
    *out_pid = 0;
    *out_type = 0;
}


Result Registration::EnableDebugForTitleId(u64 tid, Handle *out) {
    u64 old = g_debug_on_launch_tid.exchange(tid);
    if (old) {
        g_debug_on_launch_tid = old;
        return 0x80F;
    }
    *out = g_debug_title_event->GetHandle();
    return 0x0;
}

Result Registration::EnableDebugForApplication(Handle *out) {
    g_debug_next_application = true;
    *out = g_debug_application_event->GetHandle();
    return 0;
}

Result Registration::DisableDebug(u32 which) {
    if (which & 1) {
        g_debug_on_launch_tid = 0;
    }
    if (which & 2) {
        g_debug_next_application = false;
    }
    return 0;
}
