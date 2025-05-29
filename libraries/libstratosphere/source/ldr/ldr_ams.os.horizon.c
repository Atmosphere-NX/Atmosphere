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

#include <switch.h>
#include "ldr_ams.os.horizon.h"

static Result _ldrAtmosphereHasLaunchedBootProgram(Service *srv, bool *out, u64 program_id) {
    u8 tmp = 0;
    Result rc = serviceDispatchInOut(srv, 65000, program_id, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result ldrDmntAtmosphereHasLaunchedBootProgram(bool *out, u64 program_id) {
    return _ldrAtmosphereHasLaunchedBootProgram(ldrDmntGetServiceSession(), out, program_id);
}

Result ldrPmAtmosphereHasLaunchedBootProgram(bool *out, u64 program_id) {
    return _ldrAtmosphereHasLaunchedBootProgram(ldrPmGetServiceSession(), out, program_id);
}

Result ldrPmAtmosphereGetProgramInfo(LoaderProgramInfo *out_program_info, CfgOverrideStatus *out_status, const NcmProgramLocation *loc, const LoaderProgramAttributes *attr) {
    const struct {
        LoaderProgramAttributes attr;
        u16 pad1;
        u32 pad2;
        NcmProgramLocation loc;
    } in = { *attr, 0, 0, *loc };
    return serviceDispatchInOut(ldrPmGetServiceSession(), 65001, in, *out_status,
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
