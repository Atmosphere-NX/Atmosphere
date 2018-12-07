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
#include "dmnt.h"

static Service g_dmntSrv;
static u64 g_refCnt;

Result dmntInitialize(void) {
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_dmntSrv))
        return 0;

    return smGetService(&g_dmntSrv, "dmnt:-");
}

void dmntExit(void) {
    if (atomicDecrement64(&g_refCnt) == 0)
        serviceClose(&g_dmntSrv);
}

Result dmntTargetIOFileOpen(DmntFile *out, const char *path, int flags, DmntTIOCreateOption create_option) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, path, FS_MAX_PATH, BufferType_Normal);
    ipcAddRecvBuffer(&c, out, sizeof(*out), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        int flags;
        u32 create_option;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntSrv, &c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 29;
    raw->flags = flags;
    raw->create_option = create_option;
    
    Result rc = serviceIpcDispatch(&g_dmntSrv);
    
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_dmntSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result dmntTargetIOFileClose(DmntFile *f) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, f, sizeof(*f), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntSrv, &c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 30;
    
    Result rc = serviceIpcDispatch(&g_dmntSrv);
    
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_dmntSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result dmntTargetIOFileRead(DmntFile *f, u64 off, void* buf, size_t len, size_t *out_read) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, f, sizeof(*f), BufferType_Normal);
    ipcAddRecvBuffer(&c, buf, len, BufferType_Type1);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 offset;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntSrv, &c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 31;
    raw->offset = off;
    
    Result rc = serviceIpcDispatch(&g_dmntSrv);
    
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 out_read;
        } *resp;

        serviceIpcParse(&g_dmntSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        
        if (R_SUCCEEDED(rc) && out_read) {
            *out_read = resp->out_read;
        }
    }

    return rc;
}

Result dmntTargetIOFileWrite(DmntFile *f, u64 off, const void* buf, size_t len, size_t *out_written) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, f, sizeof(*f), BufferType_Normal);
    ipcAddSendBuffer(&c, buf, len, BufferType_Type1);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 offset;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntSrv, &c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 32;
    raw->offset = off;
    
    Result rc = serviceIpcDispatch(&g_dmntSrv);
    
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 out_written;
        } *resp;

        serviceIpcParse(&g_dmntSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        
        if (R_SUCCEEDED(rc) && out_written) {
            *out_written = resp->out_written;
        }
    }

    return rc;
}

Result dmntTargetIOFileGetInformation(const char *path, bool *out_is_dir, DmntFileInformation *out_info) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, path, FS_MAX_PATH, BufferType_Normal);
    ipcAddRecvBuffer(&c, out_info, sizeof(*out_info), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntSrv, &c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 34;
    
    Result rc = serviceIpcDispatch(&g_dmntSrv);
    
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            int is_dir;
        } *resp;

        serviceIpcParse(&g_dmntSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        
        if (R_SUCCEEDED(rc)) {
            *out_is_dir = resp->is_dir != 0;
        }
    }

    return rc;
}

Result dmntTargetIOFileGetSize(const char *path, u64 *out_size) {
    DmntFileInformation info;
    bool is_dir;
    Result rc = dmntTargetIOFileGetInformation(path, &is_dir, &info);
    if (R_SUCCEEDED(rc)) {
        if (is_dir) {
            /* TODO: error code? */
            rc = 0x202;
        } else {
            *out_size = info.size;
        }
    }
    return rc;
}

static Result _dmntTargetIOFileSetSize(const void *arg, size_t arg_size, u64 size) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, arg, arg_size, BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 size;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntSrv, &c, sizeof(*raw));
    
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 36;
    raw->size = size;
    
    Result rc = serviceIpcDispatch(&g_dmntSrv);
    
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_dmntSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result dmntTargetIOFileSetSize(const char *path, u64 size) {
    return _dmntTargetIOFileSetSize(path, FS_MAX_PATH, size);
}

Result dmntTargetIOFileSetOpenFileSize(DmntFile *f, u64 size) {
    /* Atmosphere extension */
    return _dmntTargetIOFileSetSize(f, sizeof(*f), size);
}
