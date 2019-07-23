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

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <malloc.h>

#include <switch.h>
#include <atmosphere.h>
#include <stratosphere.hpp>
#include <stratosphere/cfg.hpp>
#include <stratosphere/sm/sm_manager_api.hpp>

#include "pm_boot_mode_service.hpp"
#include "pm_debug_monitor_service.hpp"
#include "pm_info_service.hpp"
#include "pm_shell_service.hpp"

#include "impl/pm_process_manager.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x40000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[0x1000];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);
    void __libstratosphere_exception_handler(AtmosphereFatalErrorContext *ctx);
}

sts::ncm::TitleId __stratosphere_title_id = sts::ncm::TitleId::Pm;

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    StratosphereCrashHandler(ctx);
}

void __libnx_initheap(void) {
    void*  addr = nx_inner_heap;
    size_t size = nx_inner_heap_size;

    /* Newlib */
    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = (char*)addr;
    fake_heap_end   = (char*)addr + size;
}

namespace {

    constexpr u32 PrivilegedFileAccessHeader[0x1C / sizeof(u32)]  = {0x00000001, 0x00000000, 0x80000000, 0x0000001C, 0x00000000, 0x0000001C, 0x00000000};
    constexpr u32 PrivilegedFileAccessControl[0x2C / sizeof(u32)] = {0x00000001, 0x00000000, 0x80000000, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF};
    constexpr u8  PrivilegedServiceAccessControl[] = {0x80, '*', 0x00, '*'};
    constexpr size_t ProcessCountMax = 0x40;

    /* TODO: Libstratosphere this stuff during fatal/creport rewrite. */
    enum class DebugEventType : u32 {
        AttachProcess = 0,
        AttachThread = 1,
        ExitProcess = 2,
        ExitThread = 3,
        Exception = 4
    };

    struct AttachProcessInfo {
        sts::ncm::TitleId title_id;
        u64 process_id;
        char name[0xC];
        u32 flags;
        u64 user_exception_context_address; /* 5.0.0+ */
    };

    union DebugInfo {
        AttachProcessInfo attach_process;
    };

    struct DebugEventInfo {
        DebugEventType type;
        u32 flags;
        u64 thread_id;
        union {
            DebugInfo info;
            u64 _[0x40/sizeof(u64)];
        };
    };

    /* This uses debugging SVCs to retrieve a process's title id. */
    sts::ncm::TitleId GetProcessTitleId(u64 process_id) {
        /* Check if we should return our title id. */
        /* Doing this here works around a bug fixed in 6.0.0. */
        /* Not doing so will cause svcDebugActiveProcess to deadlock on lower firmwares if called for it's own process. */
        u64 current_process_id = 0;
        R_ASSERT(svcGetProcessId(&current_process_id, CUR_PROCESS_HANDLE));
        if (current_process_id == process_id) {
            return __stratosphere_title_id;
        }
        
        /* Get a debug handle. */
        AutoHandle debug_handle;
        if (R_FAILED(svcDebugActiveProcess(debug_handle.GetPointer(), process_id))) {    
            /* If we fail to debug a process other than our own, abort. */
            std::abort();
        }

        /* Loop until we get the event that tells us about the process. */
        DebugEventInfo d;
        while (R_SUCCEEDED(svcGetDebugEvent(reinterpret_cast<u8 *>(&d), debug_handle.Get()))) {
            if (d.type == DebugEventType::AttachProcess) {
                return d.info.attach_process.title_id;
            }
        }

        /* If we somehow didn't get the event, abort. */
        std::abort();
    }

    /* This works around a bug fixed by FS in 4.0.0. */
    /* Not doing so will cause KIPs with higher process IDs than 7 to be unable to use filesystem services. */
    /* It also registers privileged processes with SM, so that their title IDs can be known. */
    void RegisterPrivilegedProcess(u64 process_id) {
        fsprUnregisterProgram(process_id);
        fsprRegisterProgram(process_id, process_id, FsStorageId_NandSystem, PrivilegedFileAccessHeader, sizeof(PrivilegedFileAccessHeader), PrivilegedFileAccessControl, sizeof(PrivilegedFileAccessControl));
        sts::sm::manager::UnregisterProcess(process_id);
        sts::sm::manager::RegisterProcess(process_id, GetProcessTitleId(process_id), PrivilegedServiceAccessControl, sizeof(PrivilegedServiceAccessControl), PrivilegedServiceAccessControl, sizeof(PrivilegedServiceAccessControl));
    }

    void RegisterPrivilegedProcesses() {
        /* Get privileged process range. */
        u64 min_priv_process_id = 0, max_priv_process_id = 0;
        sts::cfg::GetInitialProcessRange(&min_priv_process_id, &max_priv_process_id);

        /* Get list of processes, register all privileged ones. */
        u32 num_pids;
        u64 pids[ProcessCountMax];
        R_ASSERT(svcGetProcessList(&num_pids, pids, ProcessCountMax));
        for (size_t i = 0; i < num_pids; i++) {
            if (min_priv_process_id <= pids[i] && pids[i] <= max_priv_process_id) {
                RegisterPrivilegedProcess(pids[i]);
            }
        }
    }

}

void __appInit(void) {
    SetFirmwareVersionForLibnx();

    DoWithSmSession([&]() {
        R_ASSERT(fsprInitialize());
        R_ASSERT(smManagerInitialize());

        /* This works around a bug with process permissions on < 4.0.0. */
        /* It also informs SM of privileged process information. */
        RegisterPrivilegedProcesses();

        /* Use AMS manager extension to tell SM that FS has been worked around. */
        R_ASSERT(sts::sm::manager::EndInitialDefers());

        R_ASSERT(lrInitialize());
        R_ASSERT(ldrPmInitialize());
        R_ASSERT(splInitialize());
        R_ASSERT(fsInitialize());
    });

    CheckAtmosphereVersion(CURRENT_ATMOSPHERE_VERSION);
}

void __appExit(void) {
    /* Cleanup services. */
    fsdevUnmountAll();
    fsExit();
    splExit();
    ldrPmExit();
    lrExit();
    smManagerExit();
    fsprExit();
}

int main(int argc, char **argv)
{
    /* Initialize process manager implementation. */
    R_ASSERT(sts::pm::impl::InitializeProcessManager());

    /* Create Server Manager. */
    static auto s_server_manager = WaitableManager(1);

    /* Create Services. */
    /* NOTE: Extra sessions have been added to pm:bm and pm:info to facilitate access by the rest of stratosphere. */
    /* Also Note: PM was rewritten in 5.0.0, so the shell and dmnt services are different before/after. */
    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) {
        s_server_manager.AddWaitable(new ServiceServer<sts::pm::shell::ShellService>("pm:shell", 3));
        s_server_manager.AddWaitable(new ServiceServer<sts::pm::dmnt::DebugMonitorService>("pm:dmnt", 3));
    } else {
        s_server_manager.AddWaitable(new ServiceServer<sts::pm::shell::ShellServiceDeprecated>("pm:shell", 3));
        s_server_manager.AddWaitable(new ServiceServer<sts::pm::dmnt::DebugMonitorServiceDeprecated>("pm:dmnt", 3));
    }
    s_server_manager.AddWaitable(new ServiceServer<sts::pm::bm::BootModeService>("pm:bm", 6));
    s_server_manager.AddWaitable(new ServiceServer<sts::pm::info::InformationService>("pm:info", 19));

    /* Loop forever, servicing our services. */
    s_server_manager.Process();

    return 0;
}

