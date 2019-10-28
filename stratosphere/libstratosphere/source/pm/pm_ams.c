/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "pm_ams.h"

Result pminfoAtmosphereGetProcessId(u64 *out_pid, u64 program_id) {
    return serviceDispatchInOut(pminfoGetServiceSession(), 65000, program_id, *out_pid);
}

Result pminfoAtmosphereHasLaunchedProgram(bool *out, u64 program_id) {
    u8 tmp;
    Result rc = serviceDispatchInOut(pminfoGetServiceSession(), 65001, program_id, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result pmdmntAtmosphereGetProcessInfo(Handle* handle_out, NcmProgramLocation *loc_out, u64 pid) {
    Handle tmp_handle;

    Result rc = serviceDispatchInOut(pmdmntGetServiceSession(), 65000, pid, *loc_out,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );

    if (R_SUCCEEDED(rc)) {
        if (handle_out) {
            *handle_out = tmp_handle;
        } else {
            svcCloseHandle(tmp_handle);
        }
    }

    return rc;
}

Result pmdmntAtmosphereGetCurrentLimitInfo(u64 *out_cur, u64 *out_lim, u32 group, u32 resource) {
    const struct {
        u32 group;
        u32 resource;
    } in = { group, resource };
    struct {
        u64 cur;
        u64 lim;
    } out;

    Result rc = serviceDispatchInOut(pmdmntGetServiceSession(), 65001, in, out);

    if (R_SUCCEEDED(rc)) {
        if (out_cur) *out_cur = out.cur;
        if (out_lim) *out_lim = out.lim;
    }

    return rc;
}
