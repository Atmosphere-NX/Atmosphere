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
#include <stratosphere.hpp>
#include "pm_boot_mode_service.hpp"
#include "pm_debug_monitor_service.hpp"
#include "pm_info_service.hpp"
#include "pm_shell_service.hpp"
#include "impl/pm_process_manager.hpp"

namespace ams {

    namespace pm {

        namespace {

            constexpr u32 PrivilegedFileAccessHeader[0x1C / sizeof(u32)]  = {0x00000001, 0x00000000, 0x80000000, 0x0000001C, 0x00000000, 0x0000001C, 0x00000000};
            constexpr u32 PrivilegedFileAccessControl[0x2C / sizeof(u32)] = {0x00000001, 0x00000000, 0x80000000, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF};
            constexpr u8  PrivilegedServiceAccessControl[] = {0x80, '*', 0x00, '*'};
            constexpr size_t ProcessCountMax = 0x40;

            /* This uses debugging SVCs to retrieve a process's program id. */
            ncm::ProgramId GetProcessProgramId(os::ProcessId process_id) {
                /* Get a debug handle. */
                svc::Handle debug_handle;
                R_ABORT_UNLESS(svc::DebugActiveProcess(std::addressof(debug_handle), process_id.value));
                ON_SCOPE_EXIT { R_ABORT_UNLESS(svc::CloseHandle(debug_handle)); };

                /* Loop until we get the event that tells us about the process. */
                svc::DebugEventInfo d;
                while (true) {
                    R_ABORT_UNLESS(svc::GetDebugEvent(std::addressof(d), debug_handle));
                    if (d.type == svc::DebugEvent_CreateProcess) {
                        return ncm::ProgramId{d.info.create_process.program_id};
                    }
                }
            }

            /* This works around a bug fixed by FS in 4.0.0. */
            /* Not doing so will cause KIPs with higher process IDs than 7 to be unable to use filesystem services. */
            /* It also registers privileged processes with SM, so that their program ids can be known. */
            void RegisterPrivilegedProcess(os::ProcessId process_id, ncm::ProgramId program_id) {
                fsprUnregisterProgram(process_id.value);
                fsprRegisterProgram(process_id.value, process_id.value, NcmStorageId_BuiltInSystem, PrivilegedFileAccessHeader, sizeof(PrivilegedFileAccessHeader), PrivilegedFileAccessControl, sizeof(PrivilegedFileAccessControl), 0);
                sm::manager::UnregisterProcess(process_id);
                sm::manager::RegisterProcess(process_id, program_id, cfg::OverrideStatus{}, PrivilegedServiceAccessControl, sizeof(PrivilegedServiceAccessControl), PrivilegedServiceAccessControl, sizeof(PrivilegedServiceAccessControl));
            }

            void RegisterPrivilegedProcesses() {
                /* Get privileged process range. */
                os::ProcessId min_priv_process_id, max_priv_process_id;
                R_ABORT_UNLESS(svc::GetSystemInfo(std::addressof(min_priv_process_id.value), svc::SystemInfoType_InitialProcessIdRange, svc::InvalidHandle, svc::InitialProcessIdRangeInfo_Minimum));
                R_ABORT_UNLESS(svc::GetSystemInfo(std::addressof(max_priv_process_id.value), svc::SystemInfoType_InitialProcessIdRange, svc::InvalidHandle, svc::InitialProcessIdRangeInfo_Maximum));

                /* Get current process id/program id. */
                const auto cur_process_id = os::GetCurrentProcessId();
                const auto cur_program_id = os::GetCurrentProgramId();

                /* Get list of processes, register all privileged ones. */
                s32 num_pids;
                os::ProcessId pids[ProcessCountMax];
                R_ABORT_UNLESS(svc::GetProcessList(std::addressof(num_pids), reinterpret_cast<u64 *>(pids), ProcessCountMax));
                for (s32 i = 0; i < num_pids; i++) {
                    if (min_priv_process_id <= pids[i] && pids[i] <= max_priv_process_id) {
                        RegisterPrivilegedProcess(pids[i], pids[i] == cur_process_id ? cur_program_id : GetProcessProgramId(pids[i]));
                    }
                }
            }

        }

        namespace {

            /* pm:shell, pm:dmnt, pm:bm, pm:info. */
            enum PortIndex {
                PortIndex_Shell,
                PortIndex_DebugMonitor,
                PortIndex_BootMode,
                PortIndex_Information,
                PortIndex_Count,
            };

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

            constexpr size_t MaxSessions = ShellMaxSessions + DebugMonitorMaxSessions + BootModeMaxSessions + InformationMaxSessions;
            static_assert(MaxSessions == 48, "MaxSessions");

            class ServerManager final : public sf::hipc::ServerManager<PortIndex_Count, ServerOptions, MaxSessions> {
                private:
                    virtual ams::Result OnNeedsToAccept(int port_index, Server *server) override;
            };

            ServerManager g_server_manager;

            /* NOTE: Nintendo only uses an unmanaged object for boot mode service, but no pm service has any class members/state, so we'll do it for all. */
            sf::UnmanagedServiceObject<pm::impl::IShellInterface,           pm::ShellService> g_shell_service;
            sf::UnmanagedServiceObject<pm::impl::IDeprecatedShellInterface, pm::ShellService> g_deprecated_shell_service;

            sf::UnmanagedServiceObject<pm::impl::IDebugMonitorInterface,           pm::DebugMonitorService> g_dmnt_service;
            sf::UnmanagedServiceObject<pm::impl::IDeprecatedDebugMonitorInterface, pm::DebugMonitorService> g_deprecated_dmnt_service;

            sf::UnmanagedServiceObject<pm::impl::IBootModeInterface, pm::BootModeService>       g_boot_mode_service;
            sf::UnmanagedServiceObject<pm::impl::IInformationInterface, pm::InformationService> g_information_service;

            ams::Result ServerManager::OnNeedsToAccept(int port_index, Server *server) {
                switch (port_index) {
                    case PortIndex_Shell:
                        if (hos::GetVersion() >= hos::Version_5_0_0) {
                            R_RETURN(this->AcceptImpl(server, g_shell_service.GetShared()));
                        } else {
                            R_RETURN(this->AcceptImpl(server, g_deprecated_shell_service.GetShared()));
                        }
                    case PortIndex_DebugMonitor:
                        if (hos::GetVersion() >= hos::Version_5_0_0) {
                            R_RETURN(this->AcceptImpl(server, g_dmnt_service.GetShared()));
                        } else {
                            R_RETURN(this->AcceptImpl(server, g_deprecated_dmnt_service.GetShared()));
                        }
                    case PortIndex_BootMode:
                        R_RETURN(this->AcceptImpl(server, g_boot_mode_service.GetShared()));
                    case PortIndex_Information:
                        R_RETURN(this->AcceptImpl(server, g_information_service.GetShared()));
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            }

            void RegisterServices() {
                /* NOTE: Extra sessions have been added to pm:bm and pm:info to facilitate access by the rest of stratosphere. */
                R_ABORT_UNLESS(g_server_manager.RegisterServer(PortIndex_Shell, ShellServiceName, ShellMaxSessions));
                R_ABORT_UNLESS(g_server_manager.RegisterServer(PortIndex_DebugMonitor, DebugMonitorServiceName, DebugMonitorMaxSessions));
                R_ABORT_UNLESS(g_server_manager.RegisterServer(PortIndex_BootMode, BootModeServiceName, BootModeMaxSessions));
                R_ABORT_UNLESS(g_server_manager.RegisterServer(PortIndex_Information, InformationServiceName, InformationMaxSessions));
            }

            void LoopProcess() {
                g_server_manager.LoopProcess();
            }

        }

    }

    namespace hos {

        void InitializeVersionInternal(bool allow_approximate);

    }

    namespace init {

        void InitializeSystemModule() {
            /* Initialize our connection to sm. */
            R_ABORT_UNLESS(sm::Initialize());

            /* Initialize manager interfaces for fs and sm. */
            R_ABORT_UNLESS(fsprInitialize());
            R_ABORT_UNLESS(smManagerInitialize());

            /* Work around a bug with process permissions on < 4.0.0. */
            /* This registers all initial processes explicitly with both fs and sm. */
            pm::RegisterPrivilegedProcesses();

            /* Use our manager extension to tell SM that the FS bug has been worked around. */
            R_ABORT_UNLESS(sm::manager::EndInitialDefers());

            /* Wait for the true hos version to be available. */
            hos::InitializeVersionInternal(false);

            /* Now that the true hos version is available, we should once more end defers (alerting sm to the available hos version). */
            R_ABORT_UNLESS(sm::manager::EndInitialDefers());

            /* Initialize remaining services we need. */
            R_ABORT_UNLESS(ldrPmInitialize());
            spl::Initialize();

            /* Verify that we can sanely execute. */
            ams::CheckApiVersion();
        }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() { /* ... */ }

    }

    void NORETURN Exit(int rc) {
        AMS_UNUSED(rc);
        AMS_ABORT("Exit called by immortal process");
    }

    void Main() {
        /* Set thread name. */
        os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(pm, Main));
        AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(pm, Main));

        /* Initialize process manager implementation. */
        R_ABORT_UNLESS(pm::impl::InitializeProcessManager());

        /* Create Services. */
        pm::RegisterServices();

        /* Loop forever, servicing our services. */
        pm::LoopProcess();

        /* This can never be reached. */
        AMS_ASSUME(false);
    }

}
