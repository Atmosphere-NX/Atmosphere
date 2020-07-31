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
#include <stratosphere.hpp>
#include "pm_boot_mode_service.hpp"
#include "pm_debug_monitor_service.hpp"
#include "pm_info_service.hpp"
#include "pm_shell_service.hpp"

#include "impl/pm_process_manager.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x2000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[ams::os::MemoryPageSize];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Pm;

    namespace result {

        bool CallFatalOnResultAssertion = false;

    }

}

using namespace ams;

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    ams::CrashHandler(ctx);
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

    /* This uses debugging SVCs to retrieve a process's program id. */
    ncm::ProgramId GetProcessProgramId(os::ProcessId process_id) {
        /* Check if we should return our program id. */
        /* Doing this here works around a bug fixed in 6.0.0. */
        /* Not doing so will cause svcDebugActiveProcess to deadlock on lower firmwares if called for it's own process. */
        if (process_id == os::GetCurrentProcessId()) {
            return ams::CurrentProgramId;
        }

        /* Get a debug handle. */
        os::ManagedHandle debug_handle;
        R_ABORT_UNLESS(svcDebugActiveProcess(debug_handle.GetPointer(), static_cast<u64>(process_id)));

        /* Loop until we get the event that tells us about the process. */
        svc::DebugEventInfo d;
        while (true) {
            R_ABORT_UNLESS(svcGetDebugEvent(reinterpret_cast<u8 *>(&d), debug_handle.Get()));
            if (d.type == svc::DebugEvent_CreateProcess) {
                return ncm::ProgramId{d.info.create_process.program_id};
            }
        }
    }

    /* This works around a bug fixed by FS in 4.0.0. */
    /* Not doing so will cause KIPs with higher process IDs than 7 to be unable to use filesystem services. */
    /* It also registers privileged processes with SM, so that their program ids can be known. */
    void RegisterPrivilegedProcess(os::ProcessId process_id) {
        fsprUnregisterProgram(static_cast<u64>(process_id));
        fsprRegisterProgram(static_cast<u64>(process_id), static_cast<u64>(process_id), NcmStorageId_BuiltInSystem, PrivilegedFileAccessHeader, sizeof(PrivilegedFileAccessHeader), PrivilegedFileAccessControl, sizeof(PrivilegedFileAccessControl));
        sm::manager::UnregisterProcess(process_id);
        sm::manager::RegisterProcess(process_id, GetProcessProgramId(process_id), cfg::OverrideStatus{}, PrivilegedServiceAccessControl, sizeof(PrivilegedServiceAccessControl), PrivilegedServiceAccessControl, sizeof(PrivilegedServiceAccessControl));
    }

    void RegisterPrivilegedProcesses() {
        /* Get privileged process range. */
        os::ProcessId min_priv_process_id = os::InvalidProcessId, max_priv_process_id = os::InvalidProcessId;
        cfg::GetInitialProcessRange(&min_priv_process_id, &max_priv_process_id);

        /* Get list of processes, register all privileged ones. */
        s32 num_pids;
        os::ProcessId pids[ProcessCountMax];
        R_ABORT_UNLESS(svc::GetProcessList(&num_pids, reinterpret_cast<u64 *>(pids), ProcessCountMax));
        for (s32 i = 0; i < num_pids; i++) {
            if (min_priv_process_id <= pids[i] && pids[i] <= max_priv_process_id) {
                RegisterPrivilegedProcess(pids[i]);
            }
        }
    }

}

void __appInit(void) {
    hos::InitializeForStratosphere();

    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(fsprInitialize());
        R_ABORT_UNLESS(smManagerInitialize());

        /* This works around a bug with process permissions on < 4.0.0. */
        /* It also informs SM of privileged process information. */
        RegisterPrivilegedProcesses();

        /* Use AMS manager extension to tell SM that FS has been worked around. */
        R_ABORT_UNLESS(sm::manager::EndInitialDefers());

        R_ABORT_UNLESS(ldrPmInitialize());
        spl::Initialize();
    });

    ams::CheckApiVersion();
}

void __appExit(void) {
    /* Cleanup services. */
    spl::Finalize();
    ldrPmExit();
    smManagerExit();
    fsprExit();
}

namespace {

    using ServerOptions = sf::hipc::DefaultServerManagerOptions;

    constexpr sm::ServiceName ShellServiceName = sm::ServiceName::Encode("pm:shell");
    constexpr size_t          ShellMaxSessions = 8; /* Official maximum is 3. */

    constexpr sm::ServiceName DebugMonitorServiceName = sm::ServiceName::Encode("pm:dmnt");
    constexpr size_t          DebugMonitorMaxSessions = 16;

    constexpr sm::ServiceName BootModeServiceName = sm::ServiceName::Encode("pm:bm");
    constexpr size_t          BootModeMaxSessions = 8; /* Official maximum is 4. */

    constexpr sm::ServiceName InformationServiceName = sm::ServiceName::Encode("pm:info");
    constexpr size_t          InformationMaxSessions = 48 - (ShellMaxSessions + DebugMonitorMaxSessions + BootModeMaxSessions);

    static_assert(InformationMaxSessions >= 16, "InformationMaxSessions");

    /* pm:shell, pm:dmnt, pm:bm, pm:info. */
    constexpr size_t NumServers  = 4;
    constexpr size_t MaxSessions = ShellMaxSessions + DebugMonitorMaxSessions + BootModeMaxSessions + InformationMaxSessions;
    static_assert(MaxSessions == 48, "MaxSessions");
    sf::hipc::ServerManager<NumServers, ServerOptions, MaxSessions> g_server_manager;

}

int main(int argc, char **argv)
{
    /* Set thread name. */
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(pm, Main));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(pm, Main));

    /* Initialize process manager implementation. */
    R_ABORT_UNLESS(pm::impl::InitializeProcessManager());

    /* Create Services. */
    /* NOTE: Extra sessions have been added to pm:bm and pm:info to facilitate access by the rest of stratosphere. */
    /* Also Note: PM was rewritten in 5.0.0, so the shell and dmnt services are different before/after. */
    if (hos::GetVersion() >= hos::Version_5_0_0) {
        R_ABORT_UNLESS((g_server_manager.RegisterServer<pm::impl::IShellInterface, pm::ShellService>(ShellServiceName, ShellMaxSessions)));
        R_ABORT_UNLESS((g_server_manager.RegisterServer<pm::impl::IDebugMonitorInterface, pm::DebugMonitorService>(DebugMonitorServiceName, DebugMonitorMaxSessions)));
    } else {
        R_ABORT_UNLESS((g_server_manager.RegisterServer<pm::impl::IDeprecatedShellInterface, pm::ShellService>(ShellServiceName, ShellMaxSessions)));
        R_ABORT_UNLESS((g_server_manager.RegisterServer<pm::impl::IDeprecatedDebugMonitorInterface, pm::DebugMonitorService>(DebugMonitorServiceName, DebugMonitorMaxSessions)));
    }
    R_ABORT_UNLESS((g_server_manager.RegisterServer<pm::impl::IBootModeInterface, pm::BootModeService>(BootModeServiceName, BootModeMaxSessions)));
    R_ABORT_UNLESS((g_server_manager.RegisterServer<pm::impl::IInformationInterface, pm::InformationService>(InformationServiceName, InformationMaxSessions)));

    /* Loop forever, servicing our services. */
    g_server_manager.LoopProcess();

    return 0;
}

