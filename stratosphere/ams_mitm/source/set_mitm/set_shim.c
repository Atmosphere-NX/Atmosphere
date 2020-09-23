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
#include "set_shim.h"

static Result _setCmdNoInOut64(Service* srv, u64 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

static Result _setCmdNoInOutU32(Service* srv, u32 *out, u32 cmd_id) {
    return serviceDispatchOut(srv, cmd_id, *out);
}

/* Forwarding shims. */
Result setGetLanguageCodeFwd(Service *s, u64* out) {
    return _setCmdNoInOut64(s, out, 0);
}

Result setGetRegionCodeFwd(Service *s, SetRegion *out) {
    s32 code=0;
    Result rc = _setCmdNoInOutU32(s, (u32*)&code, 4);
    if (R_SUCCEEDED(rc) && out) *out = code;
    return rc;
}
