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
#include "pgl_srv_shell.hpp"
#include "pgl_srv_shell_event_observer.hpp"
#include "pgl_srv_shell_host_utils.hpp"

namespace ams::pgl::srv {

    Result ShellInterface::LaunchProgram(ams::sf::Out<os::ProcessId> out, const ncm::ProgramLocation &loc, u32 pm_flags, u8 pgl_flags) {
        return pgl::srv::LaunchProgram(out.GetPointer(), loc, pm_flags, pgl_flags);
    }

    Result ShellInterface::TerminateProcess(os::ProcessId process_id) {
        return pgl::srv::TerminateProcess(process_id);
    }

    Result ShellInterface::LaunchProgramFromHost(ams::sf::Out<os::ProcessId> out, const ams::sf::InBuffer &content_path, u32 pm_flags) {
        return pgl::srv::LaunchProgramFromHost(out.GetPointer(), reinterpret_cast<const char *>(content_path.GetPointer()), pm_flags);
    }

    Result ShellInterface::GetHostContentMetaInfo(ams::sf::Out<pgl::ContentMetaInfo> out, const ams::sf::InBuffer &content_path) {
        return pgl::srv::GetHostContentMetaInfo(out.GetPointer(), reinterpret_cast<const char *>(content_path.GetPointer()));
    }

    Result ShellInterface::GetApplicationProcessId(ams::sf::Out<os::ProcessId> out) {
        return pgl::srv::GetApplicationProcessId(out.GetPointer());
    }

    Result ShellInterface::BoostSystemMemoryResourceLimit(u64 size) {
        return pgl::srv::BoostSystemMemoryResourceLimit(size);
    }

    Result ShellInterface::IsProcessTracked(ams::sf::Out<bool> out, os::ProcessId process_id) {
        out.SetValue(pgl::srv::IsProcessTracked(process_id));
        return ResultSuccess();
    }

    Result ShellInterface::EnableApplicationCrashReport(bool enabled) {
        pgl::srv::EnableApplicationCrashReport(enabled);
        return ResultSuccess();
    }

    Result ShellInterface::IsApplicationCrashReportEnabled(ams::sf::Out<bool> out) {
        out.SetValue(pgl::srv::IsApplicationCrashReportEnabled());
        return ResultSuccess();
    }

    Result ShellInterface::EnableApplicationAllThreadDumpOnCrash(bool enabled) {
        pgl::srv::EnableApplicationAllThreadDumpOnCrash(enabled);
        return ResultSuccess();
    }

    Result ShellInterface::TriggerApplicationSnapShotDumper(SnapShotDumpType dump_type, const ams::sf::InBuffer &arg) {
        return pgl::srv::TriggerApplicationSnapShotDumper(dump_type, reinterpret_cast<const char *>(arg.GetPointer()));
    }

    Result ShellInterface::GetShellEventObserver(ams::sf::Out<std::shared_ptr<pgl::sf::IEventObserver>> out) {
        using Interface = typename pgl::sf::IEventObserver::ImplHolder<ShellEventObserver>;

        /* Allocate a new interface. */
        auto *observer_memory = this->memory_resource->Allocate(sizeof(Interface), alignof(Interface));
        AMS_ABORT_UNLESS(observer_memory != nullptr);

        /* Create the interface object. */
        new (observer_memory) Interface;

        /* Set the output. */
        out.SetValue(std::shared_ptr<pgl::sf::IEventObserver>(reinterpret_cast<Interface *>(observer_memory), [&](Interface *obj) {
            /* Destroy the object. */
            obj->~Interface();

            /* Custom deleter: use the memory resource to free. */
            this->memory_resource->Deallocate(obj, sizeof(Interface), alignof(Interface));
        }));
        return ResultSuccess();
    }

    Result ShellInterface::Command21NotImplemented(ams::sf::Out<u64> out, u32 in, const ams::sf::InBuffer &buf1, const ams::sf::InBuffer &buf2) {
        return pgl::ResultNotImplemented();
    }

}
