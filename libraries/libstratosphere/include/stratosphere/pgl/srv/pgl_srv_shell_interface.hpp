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
#include <stratosphere/pgl/sf/pgl_sf_i_shell_interface.hpp>

namespace ams::pgl::srv {

    class ShellInterface final {
        NON_COPYABLE(ShellInterface);
        NON_MOVEABLE(ShellInterface);
        private:
            MemoryResource *memory_resource;
        public:
            constexpr ShellInterface() : memory_resource(nullptr) { /* ... */ }

            void Initialize(MemoryResource *mr) {
                AMS_ASSERT(this->memory_resource == nullptr);
                this->memory_resource = mr;
            }
        public:
            /* Interface commands. */
            Result LaunchProgram(ams::sf::Out<os::ProcessId> out, const ncm::ProgramLocation &loc, u32 pm_flags, u8 pgl_flags);
            Result TerminateProcess(os::ProcessId process_id);
            Result LaunchProgramFromHost(ams::sf::Out<os::ProcessId> out, const ams::sf::InBuffer &content_path, u32 pm_flags);
            Result GetHostContentMetaInfo(ams::sf::Out<pgl::ContentMetaInfo> out, const ams::sf::InBuffer &content_path);
            Result GetApplicationProcessId(ams::sf::Out<os::ProcessId> out);
            Result BoostSystemMemoryResourceLimit(u64 size);
            Result IsProcessTracked(ams::sf::Out<bool> out, os::ProcessId process_id);
            Result EnableApplicationCrashReport(bool enabled);
            Result IsApplicationCrashReportEnabled(ams::sf::Out<bool> out);
            Result EnableApplicationAllThreadDumpOnCrash(bool enabled);
            Result TriggerApplicationSnapShotDumper(SnapShotDumpType dump_type, const ams::sf::InBuffer &arg);

            Result GetShellEventObserver(ams::sf::Out<std::shared_ptr<pgl::sf::IEventObserver>> out);
            Result Command21NotImplemented(ams::sf::Out<u64> out, u32 in, const ams::sf::InBuffer &buf1, const ams::sf::InBuffer &buf2);
    };
    static_assert(pgl::sf::IsIShellInterface<ShellInterface>);

}
