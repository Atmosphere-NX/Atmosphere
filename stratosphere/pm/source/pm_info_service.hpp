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
#include <stratosphere.hpp>

namespace ams::pm::info {

    class InformationService final : public sf::IServiceObject {
        private:
            enum class CommandId {
                GetProgramId                 = 0,

                AtmosphereGetProcessId     = 65000,
                AtmosphereHasLaunchedProgram = 65001,
                AtmosphereGetProcessInfo = 65002,
            };
        private:
            /* Actual command implementations. */
            Result GetProgramId(sf::Out<ncm::ProgramId> out, os::ProcessId process_id);

            /* Atmosphere extension commands. */
            Result AtmosphereGetProcessId(sf::Out<os::ProcessId> out, ncm::ProgramId program_id);
            Result AtmosphereHasLaunchedProgram(sf::Out<bool> out, ncm::ProgramId program_id);
            Result AtmosphereGetProcessInfo(sf::Out<ncm::ProgramLocation> out_loc, sf::Out<cfg::OverrideStatus> out_status, os::ProcessId process_id);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetProgramId),

                MAKE_SERVICE_COMMAND_META(AtmosphereGetProcessId),
                MAKE_SERVICE_COMMAND_META(AtmosphereHasLaunchedProgram),
                MAKE_SERVICE_COMMAND_META(AtmosphereGetProcessInfo),
            };
    };

}
