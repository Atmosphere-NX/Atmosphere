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
 
#include <switch.h>
#include "fs_shim.h"

/* Missing fsp-srv commands. */
Result fsOpenBisStorageFwd(Service* s, FsStorage* out, u32 PartitionId) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 PartitionId;
    } *raw;

    raw = serviceIpcPrepareHeader(s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 12;
    raw->PartitionId = PartitionId;

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

Result fsOpenDataStorageByCurrentProcessFwd(Service* s, FsStorage* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 200;

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

Result fsOpenDataStorageByDataIdFwd(Service* s, FsStorageId storage_id, u64 data_id, FsStorage* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        FsStorageId storage_id;
        u64 data_id;
    } *raw;

    raw = serviceIpcPrepareHeader(s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 202;
    raw->storage_id = storage_id;
    raw->data_id = data_id;

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

/* Missing FS File commands. */
Result fsFileOperateRange(FsFile* f, u32 op_id, u64 off, u64 len, FsRangeInfo *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 op_id;
        u64 off;
        u64 len;
    } *raw;

    raw = serviceIpcPrepareHeader(&f->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->op_id = op_id;
    raw->off = off;
    raw->len = len;

    Result rc = serviceIpcDispatch(&f->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            FsRangeInfo range_info;
        } *resp;
        
        serviceIpcParse(&f->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && out) *out = resp->range_info;
    }

    return rc;
}

/* Missing FS Storage commands. */
Result fsStorageOperateRange(FsStorage* s, u32 op_id, u64 off, u64 len, FsRangeInfo *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 op_id;
        u64 off;
        u64 len;
    } *raw;

    raw = serviceIpcPrepareHeader(&s->s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->op_id = op_id;
    raw->off = off;
    raw->len = len;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            FsRangeInfo range_info;
        } *resp;
        
        serviceIpcParse(&s->s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && out) *out = resp->range_info;
    }

    return rc;
}