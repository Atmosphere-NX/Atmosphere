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
#include <stratosphere/os.hpp>
#include <stratosphere/pm.hpp>
#include <stratosphere/pgl/pgl_types.hpp>
#include <stratosphere/pgl/sf/pgl_sf_i_event_observer.hpp>

namespace ams::pgl::sf {

    class IShellInterface : public ams::sf::IServiceObject {
        protected:
            enum class CommandId {
                LaunchProgram                         = 0,
                TerminateProcess                      = 1,
                LaunchProgramFromHost                 = 2,
                GetHostContentMetaInfo                = 4,
                GetApplicationProcessId               = 5,
                BoostSystemMemoryResourceLimit        = 6,
                IsProcessTracked                      = 7,
                EnableApplicationCrashReport          = 8,
                IsApplicationCrashReportEnabled       = 9,
                EnableApplicationAllThreadDumpOnCrash = 10,
                TriggerApplicationSnapShotDumper      = 12,
                GetShellEventObserver                 = 20,
            };
        public:
            /* Actual commands. */
            virtual Result LaunchProgram(ams::sf::Out<os::ProcessId> out, const ncm::ProgramLocation &loc, u32 pm_flags, u8 pgl_flags) = 0;
            virtual Result TerminateProcess(os::ProcessId process_id) = 0;
            virtual Result LaunchProgramFromHost(ams::sf::Out<os::ProcessId> out, const ams::sf::InBuffer &content_path, u32 pm_flags) = 0;
            virtual Result GetHostContentMetaInfo(ams::sf::Out<pgl::ContentMetaInfo> out, const ams::sf::InBuffer &content_path) = 0;
            virtual Result GetApplicationProcessId(ams::sf::Out<os::ProcessId> out) = 0;
            virtual Result BoostSystemMemoryResourceLimit(u64 size) = 0;
            virtual Result IsProcessTracked(ams::sf::Out<bool> out, os::ProcessId process_id) = 0;
            virtual Result EnableApplicationCrashReport(bool enabled) = 0;
            virtual Result IsApplicationCrashReportEnabled(ams::sf::Out<bool> out) = 0;
            virtual Result EnableApplicationAllThreadDumpOnCrash(bool enabled) = 0;
            virtual Result TriggerApplicationSnapShotDumper(SnapShotDumpType dump_type, const ams::sf::InBuffer &arg) = 0;

            virtual Result GetShellEventObserver(ams::sf::Out<std::shared_ptr<pgl::sf::IEventObserver>> out) = 0;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(LaunchProgram),
                MAKE_SERVICE_COMMAND_META(TerminateProcess),
                MAKE_SERVICE_COMMAND_META(LaunchProgramFromHost),
                MAKE_SERVICE_COMMAND_META(GetHostContentMetaInfo),
                MAKE_SERVICE_COMMAND_META(GetApplicationProcessId),
                MAKE_SERVICE_COMMAND_META(BoostSystemMemoryResourceLimit),
                MAKE_SERVICE_COMMAND_META(IsProcessTracked),
                MAKE_SERVICE_COMMAND_META(EnableApplicationCrashReport),
                MAKE_SERVICE_COMMAND_META(IsApplicationCrashReportEnabled),
                MAKE_SERVICE_COMMAND_META(EnableApplicationAllThreadDumpOnCrash),
                MAKE_SERVICE_COMMAND_META(TriggerApplicationSnapShotDumper),
                MAKE_SERVICE_COMMAND_META(GetShellEventObserver),
            };
    };

}