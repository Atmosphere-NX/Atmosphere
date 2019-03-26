/*
 * Copyright (c) 2018 Atmosphère-NX
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

#include <string.h>
#include <switch.h>
#include "ns_shim.h"

/* Command forwarders. */
Result nsGetDocumentInterfaceFwd(Service* s, NsDocumentInterface* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 7999;

    Result rc = serviceIpcDispatch(s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            serviceCreateSubservice(&out->s, s, &r, 0);
        }
    }

    return rc;
}

Result nsamGetApplicationContentPathFwd(Service* s, void* out, size_t out_size, u64 app_id, FsStorageId storage_id) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, out, out_size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 storage_id;
        u64 app_id;
    } *raw;

    raw = serviceIpcPrepareHeader(s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 21;
    raw->storage_id = storage_id;
    raw->app_id = app_id;

    Result rc = serviceIpcDispatch(s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result nsamResolveApplicationContentPathFwd(Service* s, u64 title_id, FsStorageId storage_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 storage_id;
        u64 title_id;
    } *raw;

    raw = serviceIpcPrepareHeader(s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 23;
    raw->storage_id = storage_id;
    raw->title_id = title_id;

    Result rc = serviceIpcDispatch(s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result nsamGetRunningApplicationProgramIdFwd(Service* s, u64* out_tid, u64 app_id) {
    if (hosversionBefore(6, 0, 0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }

    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 app_id;
    } *raw;

    raw = serviceIpcPrepareHeader(s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 92;
    raw->app_id = app_id;

    Result rc = serviceIpcDispatch(s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;

        struct {
            u64 magic;
            u64 result;
            u64 title_id;
        } *resp;

        serviceIpcParse(s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            if (out_tid) {
                *out_tid = resp->title_id;
            }
        }
    }

    return rc;
}

/* Web forwarders */
Result nswebGetApplicationContentPath(NsDocumentInterface* doc, void* out, size_t out_size, u64 app_id, FsStorageId storage_id) {
    return nsamGetApplicationContentPathFwd(&doc->s, out, out_size, app_id, storage_id);
}

Result nswebResolveApplicationContentPath(NsDocumentInterface* doc, u64 title_id, FsStorageId storage_id) {
    return nsamResolveApplicationContentPathFwd(&doc->s, title_id, storage_id);
}

Result nswebGetRunningApplicationProgramId(NsDocumentInterface* doc, u64* out_tid, u64 app_id) {
    return nsamGetRunningApplicationProgramIdFwd(&doc->s, out_tid, app_id);
}
