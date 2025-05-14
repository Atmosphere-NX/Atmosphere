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
#include <stratosphere/ldr/ldr_types.hpp>

namespace ams::ldr::pm {

    /* Process Manager API. */
    Result CreateProcess(os::NativeHandle *out, PinId pin_id, u32 flags, os::NativeHandle reslimit, ldr::ProgramAttributes attrs);
    Result GetProgramInfo(ProgramInfo *out, const ncm::ProgramLocation &loc, ldr::ProgramAttributes attrs);
    Result PinProgram(PinId *out, const ncm::ProgramLocation &loc);
    Result UnpinProgram(PinId pin_id);
    Result SetEnabledProgramVerification(bool enabled);
    Result HasLaunchedBootProgram(bool *out, ncm::ProgramId program_id);

    /* Atmosphere extension API. */
    Result AtmosphereGetProgramInfo(ProgramInfo *out, cfg::OverrideStatus *out_status, const ncm::ProgramLocation &loc, ldr::ProgramAttributes attrs);
    Result AtmospherePinProgram(PinId *out, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &status);

}
