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
#include "pgl_srv_shell.hpp"
#include "pgl_srv_shell_event_observer.hpp"
#include "pgl_srv_shell_host_utils.hpp"
#include "pgl_srv_tipc_utils.hpp"

namespace ams::pgl::srv {

    Result ShellInterfaceCommon::LaunchProgramImpl(os::ProcessId *out, const ncm::ProgramLocation &loc, u32 pm_flags, u8 pgl_flags) {
        return pgl::srv::LaunchProgram(out, loc, pm_flags, pgl_flags);
    }

    Result ShellInterfaceCommon::TerminateProcessImpl(os::ProcessId process_id) {
        return pgl::srv::TerminateProcess(process_id);
    }

    Result ShellInterfaceCommon::LaunchProgramFromHostImpl(os::ProcessId *out, const void *content_path, size_t content_path_size, u32 pm_flags) {
        AMS_UNUSED(content_path_size);
        return pgl::srv::LaunchProgramFromHost(out, static_cast<const char *>(content_path), pm_flags);
    }

    Result ShellInterfaceCommon::GetHostContentMetaInfoImpl(pgl::ContentMetaInfo *out, const void *content_path, size_t content_path_size) {
        AMS_UNUSED(content_path_size);
        return pgl::srv::GetHostContentMetaInfo(out, static_cast<const char *>(content_path));
    }

    Result ShellInterfaceCommon::GetApplicationProcessIdImpl(os::ProcessId *out) {
        return pgl::srv::GetApplicationProcessId(out);
    }

    Result ShellInterfaceCommon::BoostSystemMemoryResourceLimitImpl(u64 size) {
        return pgl::srv::BoostSystemMemoryResourceLimit(size);
    }

    Result ShellInterfaceCommon::IsProcessTrackedImpl(bool *out, os::ProcessId process_id) {
        *out = pgl::srv::IsProcessTracked(process_id);
        return ResultSuccess();
    }

    Result ShellInterfaceCommon::EnableApplicationCrashReportImpl(bool enabled) {
        pgl::srv::EnableApplicationCrashReport(enabled);
        return ResultSuccess();
    }

    Result ShellInterfaceCommon::IsApplicationCrashReportEnabledImpl(bool *out) {
        *out = pgl::srv::IsApplicationCrashReportEnabled();
        return ResultSuccess();
    }

    Result ShellInterfaceCommon::EnableApplicationAllThreadDumpOnCrashImpl(bool enabled) {
        pgl::srv::EnableApplicationAllThreadDumpOnCrash(enabled);
        return ResultSuccess();
    }

    Result ShellInterfaceCommon::TriggerApplicationSnapShotDumperImpl(SnapShotDumpType dump_type, const void *arg, size_t arg_size) {
        AMS_UNUSED(arg_size);
        return pgl::srv::TriggerApplicationSnapShotDumper(dump_type, static_cast<const char *>(arg));
    }

    Result ShellInterfaceCmif::LaunchProgram(ams::sf::Out<os::ProcessId> out, const ncm::ProgramLocation &loc, u32 pm_flags, u8 pgl_flags) {
        return this->LaunchProgramImpl(out.GetPointer(), loc, pm_flags, pgl_flags);
    }

    Result ShellInterfaceCmif::TerminateProcess(os::ProcessId process_id) {
        return this->TerminateProcessImpl(process_id);
    }

    Result ShellInterfaceCmif::LaunchProgramFromHost(ams::sf::Out<os::ProcessId> out, const ams::sf::InBuffer &content_path, u32 pm_flags) {
        return this->LaunchProgramFromHostImpl(out.GetPointer(), content_path.GetPointer(), content_path.GetSize(), pm_flags);
    }

    Result ShellInterfaceCmif::GetHostContentMetaInfo(ams::sf::Out<pgl::ContentMetaInfo> out, const ams::sf::InBuffer &content_path) {
        return this->GetHostContentMetaInfoImpl(out.GetPointer(), content_path.GetPointer(), content_path.GetSize());
    }

    Result ShellInterfaceCmif::GetApplicationProcessId(ams::sf::Out<os::ProcessId> out) {
        return this->GetApplicationProcessIdImpl(out.GetPointer());
    }

    Result ShellInterfaceCmif::BoostSystemMemoryResourceLimit(u64 size) {
        return this->BoostSystemMemoryResourceLimitImpl(size);
    }

    Result ShellInterfaceCmif::IsProcessTracked(ams::sf::Out<bool> out, os::ProcessId process_id) {
        return this->IsProcessTrackedImpl(out.GetPointer(), process_id);
    }

    Result ShellInterfaceCmif::EnableApplicationCrashReport(bool enabled) {
        return this->EnableApplicationCrashReportImpl(enabled);
    }

    Result ShellInterfaceCmif::IsApplicationCrashReportEnabled(ams::sf::Out<bool> out) {
        return this->IsApplicationCrashReportEnabledImpl(out.GetPointer());
    }

    Result ShellInterfaceCmif::EnableApplicationAllThreadDumpOnCrash(bool enabled) {
        return this->EnableApplicationAllThreadDumpOnCrashImpl(enabled);
    }

    Result ShellInterfaceCmif::TriggerApplicationSnapShotDumper(SnapShotDumpType dump_type, const ams::sf::InBuffer &arg) {
        return this->TriggerApplicationSnapShotDumperImpl(dump_type, arg.GetPointer(), arg.GetSize());
    }

    Result ShellInterfaceCmif::GetShellEventObserver(ams::sf::Out<ams::sf::SharedPointer<pgl::sf::IEventObserver>> out) {
        /* Allocate a new interface. */
        auto session = ObjectFactory::CreateSharedEmplaced<pgl::sf::IEventObserver, ShellEventObserverCmif>(m_allocator);
        R_UNLESS(session != nullptr, pgl::ResultOutOfMemory());

        *out = std::move(session);
        return ResultSuccess();
    }

    Result ShellInterfaceCmif::Command21NotImplemented(ams::sf::Out<u64> out, u32 in, const ams::sf::InBuffer &buf1, const ams::sf::InBuffer &buf2) {
        AMS_UNUSED(out, in, buf1, buf2);
        return pgl::ResultNotImplemented();
    }

    Result ShellInterfaceTipc::LaunchProgram(ams::tipc::Out<os::ProcessId> out, const ncm::ProgramLocation loc, u32 pm_flags, u8 pgl_flags) {
        return this->LaunchProgramImpl(out.GetPointer(), loc, pm_flags, pgl_flags);
    }

    Result ShellInterfaceTipc::TerminateProcess(os::ProcessId process_id) {
        return this->TerminateProcessImpl(process_id);
    }

    Result ShellInterfaceTipc::LaunchProgramFromHost(ams::tipc::Out<os::ProcessId> out, const ams::tipc::InBuffer content_path, u32 pm_flags) {
        return this->LaunchProgramFromHostImpl(out.GetPointer(), content_path.GetPointer(), content_path.GetSize(), pm_flags);
    }

    Result ShellInterfaceTipc::GetHostContentMetaInfo(ams::tipc::Out<pgl::ContentMetaInfo> out, const ams::tipc::InBuffer content_path) {
        return this->GetHostContentMetaInfoImpl(out.GetPointer(), content_path.GetPointer(), content_path.GetSize());
    }

    Result ShellInterfaceTipc::GetApplicationProcessId(ams::tipc::Out<os::ProcessId> out) {
        return this->GetApplicationProcessIdImpl(out.GetPointer());
    }

    Result ShellInterfaceTipc::BoostSystemMemoryResourceLimit(u64 size) {
        return this->BoostSystemMemoryResourceLimitImpl(size);
    }

    Result ShellInterfaceTipc::IsProcessTracked(ams::tipc::Out<bool> out, os::ProcessId process_id) {
        return this->IsProcessTrackedImpl(out.GetPointer(), process_id);
    }

    Result ShellInterfaceTipc::EnableApplicationCrashReport(bool enabled) {
        return this->EnableApplicationCrashReportImpl(enabled);
    }

    Result ShellInterfaceTipc::IsApplicationCrashReportEnabled(ams::tipc::Out<bool> out) {
        return this->IsApplicationCrashReportEnabledImpl(out.GetPointer());
    }

    Result ShellInterfaceTipc::EnableApplicationAllThreadDumpOnCrash(bool enabled) {
        return this->EnableApplicationAllThreadDumpOnCrashImpl(enabled);
    }

    Result ShellInterfaceTipc::GetShellEventObserver(ams::tipc::OutMoveHandle out) {
        return pgl::srv::AllocateShellEventObserverForTipc(out.GetPointer());
    }

}
