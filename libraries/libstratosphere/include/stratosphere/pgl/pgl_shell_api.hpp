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

#pragma once
#include <vapours.hpp>
#include <stratosphere/pgl/pgl_types.hpp>
#include <stratosphere/pgl/pgl_event_observer.hpp>

namespace ams::pgl {

    Result Initialize();
    void   Finalize();

    Result LaunchProgram(os::ProcessId *out, const ncm::ProgramLocation &loc, u32 process_flags, u8 pgl_flags);
    Result TerminateProcess(os::ProcessId process_id);
    Result LaunchProgramFromHost(os::ProcessId *out, const char *content_path, u32 process_flags);
    Result GetHostContentMetaInfo(pgl::ContentMetaInfo *out, const char *content_path);
    Result GetApplicationProcessId(os::ProcessId *out);
    Result BoostSystemMemoryResourceLimit(u64 size);
    Result IsProcessTracked(bool *out, os::ProcessId process_id);
    Result EnableApplicationCrashReport(bool enabled);
    Result IsApplicationCrashReportEnabled(bool *out);
    Result EnableApplicationAllThreadDumpOnCrash(bool enabled);
    Result TriggerApplicationSnapShotDumper(const char *arg, SnapShotDumpType dump_type);

    Result GetEventObserver(pgl::EventObserver *out);

}
