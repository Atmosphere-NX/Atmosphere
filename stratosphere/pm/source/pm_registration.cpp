#include <switch.h>
#include <stratosphere.hpp>
#include <atomic>

#include "pm_registration.hpp"
#include "pm_process_wait.hpp"

static ProcessList g_process_list;

static HosSemaphore g_sema_finish_launch;

static HosMutex g_process_launch_mutex;
static Registration::ProcessLaunchState g_process_launch_state;

static std::atomic_bool g_debug_next_application(false);
static std::atomic<u64> g_debug_on_launch_tid(0);

static SystemEvent *g_process_event = NULL;
static SystemEvent *g_debug_title_event = NULL;
static SystemEvent *g_debug_application_event = NULL;
static SystemEvent *g_process_launch_start_event = NULL;

static const u64 g_memory_resource_limits[5][3] = {
    {0x010D00000ULL, 0x0CD500000ULL, 0x021700000ULL},
    {0x01E100000ULL, 0x080000000ULL, 0x061800000ULL},
    {0x014800000ULL, 0x0CD500000ULL, 0x01DC00000ULL},
    {0x028D00000ULL, 0x133400000ULL, 0x023800000ULL},
    {0x028D00000ULL, 0x0CD500000ULL, 0x089700000ULL}
};

/* These are the limits for LimitableResources. */
/* Memory, Threads, Events, TransferMemories, Sessions. */
static u64 g_resource_limits[3][5] = {
    {0x0, 0x1FC, 0x258, 0x80, 0x31A},
    {0x0, 0x60, 0x0, 0x20, 0x1},
    {0x0, 0x60, 0x0, 0x20, 0x5},
};

static Handle g_resource_limit_handles[3] = {0};

void Registration::InitializeSystemResources() {
    g_process_event = new SystemEvent(&IEvent::PanicCallback);
    g_debug_title_event = new SystemEvent(&IEvent::PanicCallback);
    g_debug_application_event = new SystemEvent(&IEvent::PanicCallback);
    g_process_launch_start_event = new SystemEvent(&Registration::ProcessLaunchStartCallback);
    
    /* Get memory limits. */
    u64 memory_arrangement;
    if (R_FAILED(splGetConfig(SplConfigItem_MemoryArrange, &memory_arrangement))) {
        /* TODO: panic. */
    }
    memory_arrangement &= 0x3F;
    int memory_limit_type;
    switch (memory_arrangement) {
        case 2:
            memory_limit_type = 1;
            break;
        case 3:
            memory_limit_type = 2;
            break;
        case 17:
            memory_limit_type = 3;
            break;
        case 18:
            memory_limit_type = 4;
            break;
        default:
            memory_limit_type = 0;
            break;
    }
    for (unsigned int i = 0; i < 3; i++) {
        g_resource_limits[i][0] = g_memory_resource_limits[memory_limit_type][i];
    }
    
    /* Create resource limits. */
    for (unsigned int i = 0; i < 3; i++) {
        if (i > 0) {
            if (R_FAILED(svcCreateResourceLimit(&g_resource_limit_handles[i]))) {
                /* TODO: Panic. */
            }
        } else {
            u64 out = 0;
            if (R_FAILED(svcGetInfo(&out, 9, 0, 0))) {
                /* TODO: Panic. */
            }
            g_resource_limit_handles[i] = (Handle)out;
        }
        for (unsigned int r = 0; r < 5; r++) {
            if (R_FAILED(svcSetResourceLimitLimitValue(g_resource_limit_handles[i], (LimitableResource)r, g_resource_limits[i][r]))) {
                /* TODO: Panic. */
            }
        }
    }
}

Result Registration::ProcessLaunchStartCallback(Handle *handles, size_t num_handles, u64 timeout) {
    Registration::HandleProcessLaunch();
    return 0;
}

IWaitable *Registration::GetProcessLaunchStartEvent() {
    return g_process_launch_start_event;
}

IWaitable *Registration::GetProcessList() {
    return &g_process_list;
}

void Registration::HandleProcessLaunch() {
    LoaderProgramInfo program_info = {0};
    Result rc;
    u64 launch_flags = g_process_launch_state.launch_flags;
    const u8 *acid_fac, *aci0_fah, *acid_sac, *aci0_sac;
    u64 *out_pid = g_process_launch_state.out_pid;
    u32 reslimit_idx;
    Process new_process = {0};
    new_process.tid_sid = g_process_launch_state.tid_sid;
    
    /* Check that this is a real program. */
    if (R_FAILED((rc = ldrPmGetProgramInfo(new_process.tid_sid.title_id, new_process.tid_sid.storage_id, &program_info)))) {
        goto HANDLE_PROCESS_LAUNCH_END;
    }
    
    /* Get the resource limit handle, ensure that we can launch the program. */
    if ((program_info.application_type & 3) == 1) {
        if (HasApplicationProcess()) {
            rc = 0xA0F;
            goto HANDLE_PROCESS_LAUNCH_END;
        }
        reslimit_idx = 1;
    } else {
        reslimit_idx = 2 * ((program_info.application_type & 3) == 2);
    }
    
    /* Try to register the title for launch in loader... */
    if (R_FAILED((rc = ldrPmRegisterTitle(new_process.tid_sid.title_id, new_process.tid_sid.storage_id, &new_process.ldr_queue_index)))) {
        goto HANDLE_PROCESS_LAUNCH_END;
    }
    
    /* Make sure the previous application is cleaned up. */
    if ((program_info.application_type & 3) == 1) {
        EnsureApplicationResourcesAvailable();
    }
    
    /* Try to create the process... */
    if (R_FAILED((rc = ldrPmCreateProcess((launch_flags >> 2) & 3, new_process.ldr_queue_index, g_resource_limit_handles[reslimit_idx], &new_process.handle)))) {
        goto PROCESS_CREATION_FAILED;
    }
    
    /* Get the new process's id. */
    svcGetProcessId(&new_process.pid, new_process.handle);
    
    /* Register with FS. */
    acid_fac = program_info.ac_buffer + program_info.acid_sac_size + program_info.aci0_sac_size;
    aci0_fah = acid_fac + program_info.acid_fac_size;
    if (R_FAILED((rc = fsprRegisterProgram(new_process.pid, new_process.tid_sid.title_id, new_process.tid_sid.storage_id, aci0_fah, program_info.aci0_fah_size, acid_fac, program_info.acid_fac_size)))) {
        goto FS_REGISTRATION_FAILED;
    }
    
    /* Register with PM. */
    acid_sac = program_info.ac_buffer;
    aci0_sac = program_info.ac_buffer + program_info.acid_sac_size;
    if (R_FAILED((rc = smManagerRegisterProcess(new_process.pid, acid_sac, program_info.acid_sac_size, aci0_sac, program_info.aci0_sac_size)))) {
        goto SM_REGISTRATION_FAILED;
    }
    
    /* Setup process flags. */
    if (program_info.application_type & 1) {
        new_process.flags |= 0x40;
    }
    if (launch_flags & 1) {
        new_process.flags |= 1;
    }
    if (launch_flags & 0x10) {
        new_process.flags |= 0x8;
    }
    
    /* Add process to the list. */
    Registration::AddProcessToList(&new_process);
    
    /* Signal, if relevant. */
    if (new_process.tid_sid.title_id == g_debug_on_launch_tid.load()) {
        g_debug_title_event->signal_event();
        g_debug_on_launch_tid = 0;
        rc = 0;
    } else if ((new_process.flags & 0x40) && g_debug_next_application.load()) {
        g_debug_application_event->signal_event();
        g_debug_next_application = false;
        rc = 0;
    } else if (launch_flags & 2) {
        rc = 0;
    } else {
        rc = svcStartProcess(new_process.handle, program_info.main_thread_priority, program_info.default_cpu_id, program_info.main_thread_stack_size);
        if (R_SUCCEEDED(rc)) {
            SetProcessState(new_process.pid, ProcessState_DebugDetached);
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


Result Registration::LaunchProcess(u64 title_id, FsStorageId storage_id, u64 launch_flags, u64 *out_pid) {
    Result rc;
    /* Only allow one mutex to exist. */
    g_process_launch_mutex.Lock();
    g_process_launch_state.tid_sid.title_id = title_id;
    g_process_launch_state.tid_sid.storage_id = storage_id;
    g_process_launch_state.launch_flags = launch_flags;
    g_process_launch_state.out_pid = out_pid;
    
    /* Start a launch, and wait for it to exit. */
    g_process_launch_start_event->signal_event();
    g_sema_finish_launch.Wait();
    
    rc = g_process_launch_state.result;
    
    g_process_launch_mutex.Unlock();
    return rc;
}

Result Registration::LaunchProcessByTidSid(TidSid tid_sid, u64 launch_flags, u64 *out_pid) {
    return LaunchProcess(tid_sid.title_id, tid_sid.storage_id, launch_flags, out_pid);
};

void Registration::HandleSignaledProcess(Process *process) {
    u64 tmp;
        
    /* Reset the signal. */
    svcResetSignal(process->handle);
    
    ProcessState old_state;
    old_state = process->state;
    svcGetProcessInfo(&tmp, process->handle, ProcessInfoType_ProcessState);
    process->state = (ProcessState)tmp;
    
    if (old_state == ProcessState_Crashed && process->state != ProcessState_Crashed) {
        process->flags &= ~0x4;
    }
    switch (process->state) {
        case ProcessState_Created:
        case ProcessState_DebugAttached:
        case ProcessState_Exiting:
            break;
        case ProcessState_DebugDetached:
        case ProcessState_Running:
            if (process->flags & 8) {
                process->flags &= ~0x30;
                process->flags |= 0x10;
                g_process_event->signal_event();
            }
            break;
        case ProcessState_Crashed:
            process->flags |= 6;
            g_process_event->signal_event();
            break;
        case ProcessState_Exited:
            if (process->flags & 1) {
                g_process_event->signal_event();
            } else {
                FinalizeExitedProcess(process);
            }
            break;
        case ProcessState_DebugSuspended:
            if (process->flags & 8) {
                process->flags |= 0x30;
                g_process_event->signal_event();
            }
            break;
    }
}

void Registration::FinalizeExitedProcess(Process *process) {
    g_process_list.Lock();
    /* Unregister with FS. */
    if (R_FAILED(fsprUnregisterProgram(process->pid))) {
        /* TODO: Panic. */
    }
    /* Unregister with SM. */
    if (R_FAILED(smManagerUnregisterProcess(process->pid))) {
        /* TODO: Panic. */
    }
    /* Unregister with LDR. */
    if (R_FAILED(ldrPmUnregisterTitle(process->ldr_queue_index))) {
        /* TODO: Panic. */
    }
    
    /* Remove. */
    RemoveProcessFromList(process->pid);

    g_process_list.Unlock();
}

void Registration::AddProcessToList(Process *process) {
    g_process_list.Lock();
    g_process_list.process_waiters.push_back(new ProcessWaiter(process));
    g_process_list.Unlock();
}

void Registration::RemoveProcessFromList(u64 pid) {
    g_process_list.Lock();
    /* Remove process from list. */
    for (unsigned int i = 0; i < g_process_list.process_waiters.size(); i++) {
        ProcessWaiter *pw = g_process_list.process_waiters[i];
        Registration::Process *process = pw->get_process();
        if (process->pid == pid) {      
            g_process_list.process_waiters.erase(g_process_list.process_waiters.begin() + i);
            svcCloseHandle(process->handle);
            delete pw;
            break;
        }
    }
    g_process_list.Unlock();
}

void Registration::SetProcessState(u64 pid, ProcessState new_state) {
    g_process_list.Lock();
    /* Set process state. */
    for (unsigned int i = 0; i < g_process_list.process_waiters.size(); i++) {
        ProcessWaiter *pw = g_process_list.process_waiters[i];
        Registration::Process *process = pw->get_process();
        if (process->pid == pid) {      
            process->state = new_state;
            break;
        }
    }
    g_process_list.Unlock();
}

bool Registration::HasApplicationProcess() {
    bool has_app = false;
    g_process_list.Lock();
    
    for (auto &pw : g_process_list.process_waiters) {
        if (pw->get_process()->flags & 0x40) {
            has_app = true;
            break;
        }
    }
    
    g_process_list.Unlock();
    return has_app;
}

void Registration::EnsureApplicationResourcesAvailable() {
    Handle application_reslimit_h = g_resource_limit_handles[1];
    for (unsigned int i = 0; i < 5; i++) {
        u64 result;
        do {
            if (R_FAILED(svcGetResourceLimitCurrentValue(&result, application_reslimit_h, (LimitableResource)i))) {
                return;
            }
            svcSleepThread(1000000ULL);
        } while (result);
    }
}