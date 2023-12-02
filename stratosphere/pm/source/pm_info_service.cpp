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
#include "pm_info_service.hpp"
#include "impl/pm_process_manager.hpp"
#include "impl/pm_spec.hpp"

namespace ams::pm {

    /* Overrides for libstratosphere pm::info commands. */
    namespace info {

        Result HasLaunchedBootProgram(bool *out, ncm::ProgramId program_id) {
            R_RETURN(ldr::pm::HasLaunchedBootProgram(out, program_id));
        }

    }

    /* Actual command implementations. */
    Result InformationService::GetProgramId(sf::Out<ncm::ProgramId> out, os::ProcessId process_id) {
        R_RETURN(impl::GetProgramId(out.GetPointer(), process_id));
    }

    Result InformationService::GetAppletResourceLimitCurrentValue(sf::Out<pm::ResourceLimitValue> out) {
        R_RETURN(impl::GetResourceLimitCurrentValue(out.GetPointer(), ResourceLimitGroup_Applet));
    }

    Result InformationService::GetAppletResourceLimitPeakValue(sf::Out<pm::ResourceLimitValue> out) {
        R_RETURN(impl::GetResourceLimitPeakValue(out.GetPointer(), ResourceLimitGroup_Applet));
    }

    /* Atmosphere extension commands. */
    Result InformationService::AtmosphereGetProcessId(sf::Out<os::ProcessId> out, ncm::ProgramId program_id) {
        R_RETURN(impl::GetProcessId(out.GetPointer(), program_id));
    }

    Result InformationService::AtmosphereHasLaunchedBootProgram(sf::Out<bool> out, ncm::ProgramId program_id) {
        R_RETURN(pm::info::HasLaunchedBootProgram(out.GetPointer(), program_id));
    }

    Result InformationService::AtmosphereGetProcessInfo(sf::Out<ncm::ProgramLocation> out_loc, sf::Out<cfg::OverrideStatus> out_status, os::ProcessId process_id) {
        /* NOTE: We don't need to worry about closing this handle, because it's an in-process copy, not a newly allocated handle. */
        os::NativeHandle dummy_handle;
        R_RETURN(impl::AtmosphereGetProcessInfo(&dummy_handle, out_loc.GetPointer(), out_status.GetPointer(), process_id));
    }

}
