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
#include "pm_info_service.hpp"
#include "impl/pm_process_manager.hpp"

namespace ams::pm {

    /* Overrides for libstratosphere pm::info commands. */
    namespace info {

        Result HasLaunchedProgram(bool *out, ncm::ProgramId program_id) {
            return ldr::pm::HasLaunchedProgram(out, program_id);
        }

    }

    /* Actual command implementations. */
    Result InformationService::GetProgramId(sf::Out<ncm::ProgramId> out, os::ProcessId process_id) {
        return impl::GetProgramId(out.GetPointer(), process_id);
    }

    /* Atmosphere extension commands. */
    Result InformationService::AtmosphereGetProcessId(sf::Out<os::ProcessId> out, ncm::ProgramId program_id) {
        return impl::GetProcessId(out.GetPointer(), program_id);
    }

    Result InformationService::AtmosphereHasLaunchedProgram(sf::Out<bool> out, ncm::ProgramId program_id) {
        return pm::info::HasLaunchedProgram(out.GetPointer(), program_id);
    }

    Result InformationService::AtmosphereGetProcessInfo(sf::Out<ncm::ProgramLocation> out_loc, sf::Out<cfg::OverrideStatus> out_status, os::ProcessId process_id) {
        Handle dummy_handle;
        return impl::AtmosphereGetProcessInfo(&dummy_handle, out_loc.GetPointer(), out_status.GetPointer(), process_id);
    }

}
