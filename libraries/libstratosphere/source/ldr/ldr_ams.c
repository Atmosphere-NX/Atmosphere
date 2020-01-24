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

#include <switch.h>
#include "ldr_ams.h"

static Result _ldrAtmosphereHasLaunchedProgram(Service *srv, bool *out, u64 program_id) {
    u8 tmp;
    Result rc = serviceDispatchInOut(srv, 65000, program_id, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result ldrDmntAtmosphereHasLaunchedProgram(bool *out, u64 program_id) {
    return _ldrAtmosphereHasLaunchedProgram(ldrDmntGetServiceSession(), out, program_id);
}

Result ldrPmAtmosphereHasLaunchedProgram(bool *out, u64 program_id) {
    return _ldrAtmosphereHasLaunchedProgram(ldrPmGetServiceSession(), out, program_id);
}

Result ldrPmAtmosphereGetProgramInfo(LoaderProgramInfo *out_program_info, CfgOverrideStatus *out_status, const NcmProgramLocation *loc) {
    return serviceDispatchInOut(ldrPmGetServiceSession(), 65001, *loc, *out_status,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcPointer | SfBufferAttr_FixedSize },
        .buffers = { { out_program_info, sizeof(*out_program_info) } },
    );
}

Result ldrPmAtmospherePinProgram(u64 *out, const NcmProgramLocation *loc, const CfgOverrideStatus *status) {
    const struct {
        NcmProgramLocation loc;
        CfgOverrideStatus status;
    } in = { *loc, *status };
    return serviceDispatchInOut(ldrPmGetServiceSession(), 65002, in, *out);
}
