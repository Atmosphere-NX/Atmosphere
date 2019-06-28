/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include <atomic>
#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/ldr.hpp>
#include <stratosphere/ldr/ldr_pm_api.hpp>

#include "pm_registration.hpp"
#include "pm_resource_limits.hpp"

static ProcessList g_process_list;
static ProcessList g_dead_process_list;

static IEvent *g_process_launch_start_event = nullptr;
static HosSignal g_finish_launch_signal;

static HosMutex g_process_launch_mutex;
static Registration::ProcessLaunchState g_process_launch_state;

static std::atomic_bool g_debug_next_application(false);
static std::atomic<u64> g_debug_on_launch_tid(0);

static IEvent *g_process_event = nullptr;
static IEvent *g_debug_title_event = nullptr;
static IEvent *g_debug_application_event = nullptr;
static IEvent *g_boot_finished_event = nullptr;

ProcessList &Registration::GetProcessList() {
    return g_process_list;
}

void Registration::InitializeSystemResources() {
    g_process_event = CreateWriteOnlySystemEvent();
    g_debug_title_event = CreateWriteOnlySystemEvent();
    g_debug_application_event = CreateWriteOnlySystemEvent();
    g_boot_finished_event = CreateWriteOnlySystemEvent();

    /* Auto-clear non-system event. */
    g_process_launch_start_event = CreateHosEvent(&Registration::ProcessLaunchStartCallback);

    ResourceLimitUtils::InitializeLimits();
}

Result Registration::ProcessLaunchStartCallback(u64 timeout) {
    g_process_launch_start_event->Clear();
    Registration::HandleProcessLaunch();
    return ResultSuccess;
}

IWaitable *Registration::GetProcessLaunchStartEvent() {
    return g_process_launch_start_event;
}

Result Registration::LaunchProcess(u64 *out_pid, const TidSid tid_sid, const u64 launch_flags) {
    LoaderProgramInfo program_info = {0};
    Process new_process = {0};
    new_process.tid_sid = tid_sid;

    /* Clear, get accessors for access controls. */
    static u8 s_ac_buf[4 * sizeof(LoaderProgramInfo)];
    std::memset(s_ac_buf, 0xCC, sizeof(s_ac_buf));
    u8 *acid_sac = s_ac_buf, *aci0_sac = acid_sac + sizeof(LoaderProgramInfo), *fac = aci0_sac + sizeof(LoaderProgramInfo), *fah = fac + sizeof(LoaderProgramInfo);

    /* Check that this is a real program. */
    R_TRY(ldrPmGetProgramInfo(new_process.tid_sid.title_id, new_process.tid_sid.storage_id, &program_info));

    /* Get the resource limit handle, ensure that we can launch the program. */
    if ((program_info.application_type & 3) == 1 && HasApplicationProcess()) {
        return ResultPmApplicationRunning;
    }

    /* Try to register the title for launch in loader... */
    R_TRY(ldrPmRegisterTitle(new_process.tid_sid.title_id, new_process.tid_sid.storage_id, &new_process.ldr_queue_index));
    auto ldr_register_guard = SCOPE_GUARD {
        ldrPmUnregisterTitle(new_process.ldr_queue_index);
    };

    /* Make sure the previous application is cleaned up. */
    if ((program_info.application_type & 3) == 1) {
        ResourceLimitUtils::EnsureApplicationResourcesAvailable();
    }

    /* Try to create the process... */
    R_TRY(ldrPmCreateProcess(LAUNCHFLAGS_ARGLOW(launch_flags) | LAUNCHFLAGS_ARGHIGH(launch_flags), new_process.ldr_queue_index, ResourceLimitUtils::GetResourceLimitHandle(program_info.application_type), &new_process.handle));
    auto proc_create_guard = SCOPE_GUARD {
        svcCloseHandle(new_process.handle);
        new_process.handle = 0;
    };

    /* Get the new process's id. */
    R_ASSERT(svcGetProcessId(&new_process.pid, new_process.handle));

    /* Register with FS. */
    memcpy(fac, program_info.ac_buffer + program_info.acid_sac_size + program_info.aci0_sac_size, program_info.acid_fac_size);
    memcpy(fah, program_info.ac_buffer + program_info.acid_sac_size + program_info.aci0_sac_size + program_info.acid_fac_size, program_info.aci0_fah_size);
    R_TRY(fsprRegisterProgram(new_process.pid, new_process.tid_sid.title_id, new_process.tid_sid.storage_id, fah, program_info.aci0_fah_size, fac, program_info.acid_fac_size));
    auto fs_register_guard = SCOPE_GUARD {
        fsprUnregisterProgram(new_process.pid);
    };

    /* Register with SM. */
    memcpy(acid_sac, program_info.ac_buffer, program_info.acid_sac_size);
    memcpy(aci0_sac, program_info.ac_buffer + program_info.acid_sac_size, program_info.aci0_sac_size);
    R_TRY(smManagerRegisterProcess(new_process.pid, acid_sac, program_info.acid_sac_size, aci0_sac, program_info.aci0_sac_size));
    auto sm_register_guard = SCOPE_GUARD {
        smManagerUnregisterProcess(new_process.pid);
    };

    /* Setup process flags. */
    if (program_info.application_type & 1) {
        new_process.flags |= PROCESSFLAGS_APPLICATION;
    }
    if ((GetRuntimeFirmwareVersion() >= FirmwareVersion_200) && LAUNCHFLAGS_NOTIYDEBUGSPECIAL(launch_flags) && (program_info.application_type & 4)) {
        new_process.flags |= PROCESSFLAGS_NOTIFYDEBUGSPECIAL;
    }
    if (LAUNCHFLAGS_NOTIFYWHENEXITED(launch_flags)) {
        new_process.flags |= PROCESSFLAGS_NOTIFYWHENEXITED;
    }
    if (LAUNCHFLAGS_NOTIFYDEBUGEVENTS(launch_flags) && (GetRuntimeFirmwareVersion() < FirmwareVersion_200 || (program_info.application_type & 4))) {
        new_process.flags |= PROCESSFLAGS_NOTIFYDEBUGEVENTS;
    }

    /* Add process to the list. */
    Registration::AddProcessToList(std::make_shared<Registration::Process>(new_process));
    auto reg_list_guard = SCOPE_GUARD {
        Registration::RemoveProcessFromList(new_process.pid);
    };

    /* Signal, if relevant. */
    if (new_process.tid_sid.title_id == g_debug_on_launch_tid.load()) {
        g_debug_title_event->Signal();
        g_debug_on_launch_tid = 0;
    } else if ((new_process.flags & PROCESSFLAGS_APPLICATION) && g_debug_next_application.load()) {
        g_debug_application_event->Signal();
        g_debug_next_application = false;
    } else if (!(LAUNCHFLAGS_STARTSUSPENDED(launch_flags))) {
        R_TRY(svcStartProcess(new_process.handle, program_info.main_thread_priority, program_info.default_cpu_id, program_info.main_thread_stack_size));
        SetProcessState(new_process.pid, ProcessState_Running);
    }

    /* Dismiss scope guards. */
    reg_list_guard.Cancel();
    sm_register_guard.Cancel();
    fs_register_guard.Cancel();
    proc_create_guard.Cancel();
    ldr_register_guard.Cancel();

    /* Set output pid. */
    *out_pid = new_process.pid;

    return ResultSuccess;
}

void Registration::HandleProcessLaunch() {
    /* Actually launch process. */
    g_process_launch_state.result = LaunchProcess(g_process_launch_state.out_pid, g_process_launch_state.tid_sid, g_process_launch_state.launch_flags);
    /* Signal to waiting thread. */
    g_finish_launch_signal.Signal();
}


Result Registration::LaunchDebugProcess(u64 pid) {
    std::scoped_lock<ProcessList &> lk(GetProcessList());
    LoaderProgramInfo program_info = {0};

    std::shared_ptr<Registration::Process> proc = GetProcess(pid);
    if (proc == NULL) {
        return ResultPmProcessNotFound;
    }

    if (proc->state >= ProcessState_Running) {
        return ResultPmAlreadyStarted;
    }

    /* Check that this is a real program. */
    R_TRY(ldrPmGetProgramInfo(proc->tid_sid.title_id, proc->tid_sid.storage_id, &program_info));

    /* Actually start the process. */
    R_TRY(svcStartProcess(proc->handle, program_info.main_thread_priority, program_info.default_cpu_id, program_info.main_thread_stack_size));

    proc->state = ProcessState_Running;
    return ResultSuccess;
}

Result Registration::LaunchProcess(u64 title_id, FsStorageId storage_id, u64 launch_flags, u64 *out_pid) {
    /* Only allow one mutex to exist. */
    std::scoped_lock lk{g_process_launch_mutex};
    g_process_launch_state.tid_sid.title_id = title_id;
    g_process_launch_state.tid_sid.storage_id = storage_id;
    g_process_launch_state.launch_flags = launch_flags;
    g_process_launch_state.out_pid = out_pid;

    /* Start a launch, and wait for it to exit. */
    g_finish_launch_signal.Reset();
    g_process_launch_start_event->Signal();
    g_finish_launch_signal.Wait();

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
            if ((GetRuntimeFirmwareVersion() >= FirmwareVersion_200) && process->flags & PROCESSFLAGS_NOTIFYDEBUGSPECIAL) {
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
            if (process->flags & PROCESSFLAGS_NOTIFYWHENEXITED && GetRuntimeFirmwareVersion() < FirmwareVersion_500) {
                g_process_event->Signal();
            } else {
                FinalizeExitedProcess(process);
            }
            return ResultKernelConnectionClosed;
        case ProcessState_DebugSuspended:
            if (process->flags & PROCESSFLAGS_NOTIFYDEBUGEVENTS) {
                process->flags |= (PROCESSFLAGS_DEBUGEVENTPENDING | PROCESSFLAGS_DEBUGSUSPENDED);
                g_process_event->Signal();
            }
            break;
    }
    return ResultSuccess;
}

void Registration::FinalizeExitedProcess(std::shared_ptr<Registration::Process> process) {
    bool signal_debug_process_5x;

    /* Scope to manage process list lock. */
    {
        std::scoped_lock<ProcessList &> lk(GetProcessList());

        signal_debug_process_5x = (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) && process->flags & PROCESSFLAGS_NOTIFYWHENEXITED;

        /* Unregister with FS, SM, and LDR. */
        R_ASSERT(fsprUnregisterProgram(process->pid));
        R_ASSERT(smManagerUnregisterProcess(process->pid));
        R_ASSERT(ldrPmUnregisterTitle(process->ldr_queue_index));

        /* Close the process's handle. */
        svcCloseHandle(process->handle);
        process->handle = 0;

        /* Insert into dead process list, if relevant. */
        if (signal_debug_process_5x) {
            std::scoped_lock<ProcessList &> dead_lk(g_dead_process_list);

            g_dead_process_list.processes.push_back(process);
        }

        /* Remove NOTE: This probably frees process. */
        RemoveProcessFromList(process->pid);
    }

    if (signal_debug_process_5x) {
        g_process_event->Signal();
    }
}

void Registration::AddProcessToList(std::shared_ptr<Registration::Process> process) {
    std::scoped_lock<ProcessList &> lk(GetProcessList());

    g_process_list.processes.push_back(process);
    g_process_launch_start_event->GetManager()->AddWaitable(new ProcessWaiter(process));
}

void Registration::RemoveProcessFromList(u64 pid) {
    std::scoped_lock<ProcessList &> lk(GetProcessList());

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
    std::scoped_lock<ProcessList &> lk(GetProcessList());

    /* Set process state. */
    for (auto &process : g_process_list.processes) {
        if (process->pid == pid) {
            process->state = new_state;
            break;
        }
    }
}

bool Registration::HasApplicationProcess(std::shared_ptr<Registration::Process> *out) {
    std::scoped_lock<ProcessList &> lk(GetProcessList());

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
    std::scoped_lock<ProcessList &> lk(GetProcessList());

    for (auto &process : g_process_list.processes) {
        if (process->pid == pid) {
            return process;
        }
    }

    return nullptr;
}

std::shared_ptr<Registration::Process> Registration::GetProcessByTitleId(u64 tid) {
    std::scoped_lock<ProcessList &> lk(GetProcessList());

    for (auto &process : g_process_list.processes) {
        if (process->tid_sid.title_id == tid) {
            return process;
        }
    }

    return nullptr;
}


Result Registration::GetDebugProcessIds(u64 *out_pids, u32 max_out, u32 *num_out) {
    std::scoped_lock<ProcessList &> lk(GetProcessList());
    u32 num = 0;


    for (auto &process : g_process_list.processes) {
        if (process->flags & PROCESSFLAGS_CRASH_DEBUG && num < max_out) {
            out_pids[num++] = process->pid;
        }
    }

    *num_out = num;
    return ResultSuccess;
}

Handle Registration::GetProcessEventHandle() {
    return g_process_event->GetHandle();
}

void Registration::GetProcessEventType(u64 *out_pid, u64 *out_type) {
    /* Scope to manage process list lock. */
    {
        std::scoped_lock<ProcessList &> lk(GetProcessList());

        for (auto &p : g_process_list.processes) {
            if ((GetRuntimeFirmwareVersion() >= FirmwareVersion_200) && p->state >= ProcessState_Running && p->flags & PROCESSFLAGS_DEBUGDETACHED) {
                p->flags &= ~PROCESSFLAGS_DEBUGDETACHED;
                *out_pid = p->pid;
                *out_type = (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) ? PROCESSEVENTTYPE_500_DEBUGDETACHED : PROCESSEVENTTYPE_DEBUGDETACHED;
                return;
            }
            if (p->flags & PROCESSFLAGS_DEBUGEVENTPENDING) {
                u64 old_flags = p->flags;
                p->flags &= ~PROCESSFLAGS_DEBUGEVENTPENDING;
                *out_pid = p->pid;
                *out_type = (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) ?
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
                *out_type = (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) ? PROCESSEVENTTYPE_500_CRASH : PROCESSEVENTTYPE_CRASH;
                return;
            }
            if (GetRuntimeFirmwareVersion() < FirmwareVersion_500 && p->flags & PROCESSFLAGS_NOTIFYWHENEXITED && p->state == ProcessState_Exited) {
                *out_pid = p->pid;
                *out_type = PROCESSEVENTTYPE_EXIT;
                return;
            }
        }

        *out_pid = 0;
        *out_type = 0;
    }

    if ((GetRuntimeFirmwareVersion() >= FirmwareVersion_500)) {
        std::scoped_lock<ProcessList &> dead_lk(g_dead_process_list);

        if (g_dead_process_list.processes.size()) {
            std::shared_ptr<Registration::Process> process = g_dead_process_list.processes[0];
            g_dead_process_list.processes.erase(g_dead_process_list.processes.begin());
            *out_pid = process->pid;
            *out_type = PROCESSEVENTTYPE_500_EXIT;
            return;
        }
    }
}


Result Registration::EnableDebugForTitleId(u64 tid, Handle *out) {
    u64 old = g_debug_on_launch_tid.exchange(tid);
    if (old) {
        g_debug_on_launch_tid = old;
        return ResultPmDebugHookInUse;
    }
    *out = g_debug_title_event->GetHandle();
    return ResultSuccess;
}

Result Registration::EnableDebugForApplication(Handle *out) {
    g_debug_next_application = true;
    *out = g_debug_application_event->GetHandle();
    return ResultSuccess;
}

Result Registration::DisableDebug(u32 which) {
    if (which & 1) {
        g_debug_on_launch_tid = 0;
    }
    if (which & 2) {
        g_debug_next_application = false;
    }
    return ResultSuccess;
}

Handle Registration::GetDebugTitleEventHandle() {
    return g_debug_title_event->GetHandle();
}

Handle Registration::GetDebugApplicationEventHandle() {
    return g_debug_application_event->GetHandle();
}

Handle Registration::GetBootFinishedEventHandle() {
    return g_boot_finished_event->GetHandle();
}

bool Registration::HasLaunchedTitle(u64 title_id) {
    bool has_launched = false;
    R_ASSERT(sts::ldr::pm::HasLaunchedTitle(&has_launched, sts::ncm::TitleId{title_id}));
    return has_launched;
}

void Registration::SignalBootFinished() {
    g_boot_finished_event->Signal();
}
