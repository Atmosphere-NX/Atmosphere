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
#include "ldr_ams.h"

namespace ams::ldr::pm {

    /* Information API. */
    Result CreateProcess(os::NativeHandle *out, PinId pin_id, u32 flags, Handle reslimit) {
        return ldrPmCreateProcess(pin_id.value, flags, reslimit, out);
    }

    Result GetProgramInfo(ProgramInfo *out, const ncm::ProgramLocation &loc) {
        return ldrPmGetProgramInfo(reinterpret_cast<const NcmProgramLocation *>(std::addressof(loc)), reinterpret_cast<LoaderProgramInfo *>(out));
    }

    Result PinProgram(PinId *out, const ncm::ProgramLocation &loc) {
        static_assert(sizeof(*out) == sizeof(u64), "PinId definition!");
        return ldrPmPinProgram(reinterpret_cast<const NcmProgramLocation *>(std::addressof(loc)), reinterpret_cast<u64 *>(out));
    }

    Result UnpinProgram(PinId pin_id) {
        return ldrPmUnpinProgram(pin_id.value);
    }

    Result HasLaunchedBootProgram(bool *out, ncm::ProgramId program_id) {
        return ldrPmAtmosphereHasLaunchedBootProgram(out, static_cast<u64>(program_id));
    }

    Result AtmosphereGetProgramInfo(ProgramInfo *out, cfg::OverrideStatus *out_status, const ncm::ProgramLocation &loc) {
        static_assert(sizeof(*out_status) == sizeof(CfgOverrideStatus), "CfgOverrideStatus definition!");
        return ldrPmAtmosphereGetProgramInfo(reinterpret_cast<LoaderProgramInfo *>(out), reinterpret_cast<CfgOverrideStatus *>(out_status), reinterpret_cast<const NcmProgramLocation *>(std::addressof(loc)));
    }

    Result SetEnabledProgramVerification(bool enabled) {
        return ldrPmSetEnabledProgramVerification(enabled);
    }

    Result AtmospherePinProgram(PinId *out, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &status) {
        static_assert(sizeof(*out) == sizeof(u64), "PinId definition!");
        static_assert(sizeof(status) == sizeof(CfgOverrideStatus), "CfgOverrideStatus definition!");
        return ldrPmAtmospherePinProgram(reinterpret_cast<u64 *>(out), reinterpret_cast<const NcmProgramLocation *>(std::addressof(loc)), reinterpret_cast<const CfgOverrideStatus *>(std::addressof(status)));
    }

}
