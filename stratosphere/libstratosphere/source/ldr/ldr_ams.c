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
#include "ldr_ams.h"

static Result _ldrAtmosphereHasLaunchedTitle(Service *srv, bool *out, u64 tid) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 title_id;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65000;
    raw->title_id = tid;

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 has_launched_title;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->has_launched_title != 0;
        } else {
            rc = 0x666;
        }
    } else {
        rc = 0x555;
    }

    return rc;
}

Result ldrDmntAtmosphereHasLaunchedTitle(bool *out, u64 tid) {
    return _ldrAtmosphereHasLaunchedTitle(ldrDmntGetServiceSession(), out, tid);
}

Result ldrPmAtmosphereHasLaunchedTitle(bool *out, u64 tid) {
    return _ldrAtmosphereHasLaunchedTitle(ldrPmGetServiceSession(), out, tid);
}
