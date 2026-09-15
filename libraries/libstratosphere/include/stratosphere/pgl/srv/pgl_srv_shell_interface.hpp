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
#pragma once
#include <vapours.hpp>
#include <stratosphere/pgl/pgl_types.hpp>
#include <stratosphere/pgl/sf/pgl_sf_i_shell_interface.hpp>
#include <stratosphere/pgl/tipc/pgl_tipc_i_shell_interface.hpp>

namespace ams::pgl::srv {

    class ShellInterfaceCommon {
        NON_COPYABLE(ShellInterfaceCommon);
        NON_MOVEABLE(ShellInterfaceCommon);
        public:
            constexpr ShellInterfaceCommon() = default;
        public:
            Result LaunchProgramImpl(os::ProcessId *out, const ncm::ProgramLocation &loc, u32 pm_flags, u8 pgl_flags);
            Result TerminateProcessImpl(os::ProcessId process_id);
            Result LaunchProgramFromHostImpl(os::ProcessId *out, const void *content_path, size_t content_path_size, u32 pm_flags);
            Result GetProgramLaunchPropertyFromHostImpl(pgl::ProgramLaunchProperty *out, const void *content_path, size_t content_path_size);
            Result GetRunningApplicationProcessIdImpl(os::ProcessId *out);
            Result BoostSystemMemoryResourceLimitImpl(u64 size);
            Result IsRunningProcessImpl(bool *out, os::ProcessId process_id);
            Result EnableApplicationCrashReportImpl(bool enabled);
            Result IsApplicationCrashReportEnabledImpl(bool *out);
            Result EnableApplicationAllThreadDumpOnCrashImpl(bool enabled);
            Result GetProcessIdImpl(os::ProcessId *out, ncm::ProgramId program_id);
            Result TriggerSnapShotDumperImpl(SnapShotDumpType dump_type, const void *arg, size_t arg_size);
            Result EnableApplicationCrashReport2Impl(os::ProcessId process_id, bool enabled);
    };

    class ShellInterfaceCmif : public ShellInterfaceCommon {
        NON_COPYABLE(ShellInterfaceCmif);
        NON_MOVEABLE(ShellInterfaceCmif);
        private:
            using Allocator     = ams::sf::ExpHeapAllocator;
            using ObjectFactory = ams::sf::ObjectFactory<ams::sf::ExpHeapAllocator::Policy>;
        private:
            Allocator *m_allocator;
        public:
            constexpr ShellInterfaceCmif(Allocator *a) : ShellInterfaceCommon(), m_allocator(a) { /* ... */ }
        public:
            /* Interface commands. */
            Result LaunchProgram(ams::sf::Out<os::ProcessId> out, const ncm::ProgramLocation &loc, u32 pm_flags, u8 pgl_flags);
            Result TerminateProcess(os::ProcessId process_id);
            Result LaunchProgramFromHost(ams::sf::Out<os::ProcessId> out, const ams::sf::InBuffer &content_path, u32 pm_flags);
            Result GetProgramLaunchPropertyFromHost(ams::sf::Out<pgl::ProgramLaunchProperty> out, const ams::sf::InBuffer &content_path);
            Result GetRunningApplicationProcessId(ams::sf::Out<os::ProcessId> out);
            Result BoostSystemMemoryResourceLimit(u64 size);
            Result IsRunningProcess(ams::sf::Out<bool> out, os::ProcessId process_id);
            Result EnableApplicationCrashReport(bool enabled);
            Result IsApplicationCrashReportEnabled(ams::sf::Out<bool> out);
            Result EnableApplicationAllThreadDumpOnCrash(bool enabled);
            Result TriggerSnapShotDumper(SnapShotDumpType dump_type, const ams::sf::InBuffer &arg);
            Result CreateShellEvent(ams::sf::Out<ams::sf::SharedPointer<pgl::sf::IEventObserver>> out);
            Result Command21NotImplemented(ams::sf::Out<u64> out, u32 in, const ams::sf::InBuffer &buf1, const ams::sf::InBuffer &buf2);
    };
    static_assert(pgl::sf::IsIShellInterface<ShellInterfaceCmif>);

    class ShellInterfaceTipc : public ShellInterfaceCommon {
        NON_COPYABLE(ShellInterfaceTipc);
        NON_MOVEABLE(ShellInterfaceTipc);
        public:
            constexpr ShellInterfaceTipc() : ShellInterfaceCommon() { /* ... */ }
        public:
            /* Interface commands. */
            Result LaunchProgram(ams::tipc::Out<os::ProcessId> out, const ncm::ProgramLocation loc, u32 pm_flags, u8 pgl_flags);
            Result TerminateProcess(os::ProcessId process_id);
            Result LaunchProgramFromHost(ams::tipc::Out<os::ProcessId> out, const ams::tipc::InBuffer content_path, u32 pm_flags);
            Result GetProgramLaunchPropertyFromHost(ams::tipc::Out<pgl::ProgramLaunchProperty> out, const ams::tipc::InBuffer content_path);
            Result GetRunningApplicationProcessId(ams::tipc::Out<os::ProcessId> out);
            Result BoostSystemMemoryResourceLimit(u64 size);
            Result IsRunningProcess(ams::tipc::Out<bool> out, os::ProcessId process_id);
            Result EnableApplicationCrashReport(bool enabled);
            Result IsApplicationCrashReportEnabled(ams::tipc::Out<bool> out);
            Result EnableApplicationAllThreadDumpOnCrash(bool enabled);
            Result GetProcessId(ams::tipc::Out<os::ProcessId> out, ncm::ProgramId program_id);
            Result CreateShellEvent(ams::tipc::OutMoveHandle out);
            Result EnableApplicationCrashReport2(os::ProcessId process_id, bool enabled);
    };
    static_assert(pgl::tipc::IsIShellInterface<ShellInterfaceTipc>);

}
