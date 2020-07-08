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
#include "pgl_remote_event_observer.hpp"

namespace ams::pgl {

    Result Initialize() {
        return ::pglInitialize();
    }

    void Finalize() {
        return pglExit();
    }

    Result LaunchProgram(os::ProcessId *out, const ncm::ProgramLocation &loc, u32 process_flags, u8 pgl_flags) {
        static_assert(sizeof(*out) == sizeof(u64));
        static_assert(sizeof(loc)  == sizeof(::NcmProgramLocation));
        return ::pglLaunchProgram(reinterpret_cast<u64 *>(out), reinterpret_cast<const ::NcmProgramLocation *>(std::addressof(loc)), process_flags, pgl_flags);
    }

    Result TerminateProcess(os::ProcessId process_id) {
        return ::pglTerminateProcess(static_cast<u64>(process_id));
    }

    Result LaunchProgramFromHost(os::ProcessId *out, const char *content_path, u32 process_flags) {
        static_assert(sizeof(*out) == sizeof(u64));
        return ::pglLaunchProgramFromHost(reinterpret_cast<u64 *>(out), content_path, process_flags);
    }

    Result GetHostContentMetaInfo(pgl::ContentMetaInfo *out, const char *content_path) {
        static_assert(sizeof(*out) == sizeof(::PglContentMetaInfo));
        return ::pglGetHostContentMetaInfo(reinterpret_cast<::PglContentMetaInfo *>(out), content_path);
    }

    Result GetApplicationProcessId(os::ProcessId *out) {
        static_assert(sizeof(*out) == sizeof(u64));
        return ::pglGetApplicationProcessId(reinterpret_cast<u64 *>(out));
    }

    Result BoostSystemMemoryResourceLimit(u64 size) {
        return ::pglBoostSystemMemoryResourceLimit(size);
    }

    Result IsProcessTracked(bool *out, os::ProcessId process_id) {
        return ::pglIsProcessTracked(out, static_cast<u64>(process_id));
    }

    Result EnableApplicationCrashReport(bool enabled) {
        return ::pglEnableApplicationCrashReport(enabled);
    }

    Result IsApplicationCrashReportEnabled(bool *out) {
        return ::pglIsApplicationCrashReportEnabled(out);
    }

    Result EnableApplicationAllThreadDumpOnCrash(bool enabled) {
        return ::pglEnableApplicationAllThreadDumpOnCrash(enabled);
    }

    Result TriggerApplicationSnapShotDumper(const char *arg, SnapShotDumpType dump_type) {
        return ::pglTriggerApplicationSnapShotDumper(static_cast<::PglSnapShotDumpType>(dump_type), arg);
    }

    Result GetEventObserver(pgl::EventObserver *out) {
        ::PglEventObserver obs;
        R_TRY(::pglGetEventObserver(std::addressof(obs)));

        auto remote_observer = ams::sf::MakeShared<pgl::sf::IEventObserver, RemoteEventObserver>(obs);
        AMS_ABORT_UNLESS(remote_observer != nullptr);

        *out = pgl::EventObserver(remote_observer);
        return ResultSuccess();
    }

}