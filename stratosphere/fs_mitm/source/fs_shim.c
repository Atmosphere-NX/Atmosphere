#include <switch.h>
#include "fs_shim.h"

/* Necessary evil. */
Result ipcCopyFromDomain(Handle session, u32 object_id, Service *out) {
    u32* buf = (u32*)armGetTls();

    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u32 object_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    buf[0] = IpcCommandType_Control;
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->object_id = object_id;

    Result rc = ipcDispatch(session);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct ipcCopyFromDomainResponse {
            u64 magic;
            u64 result;
        } *raw = (struct ipcCopyFromDomainResponse*)r.Raw;

        rc = raw->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreate(out, r.Handles[0]);
        }
    }

    return rc;
}


/* Missing fsp-srv commands. */
Result fsOpenDataStorageByCurrentProcessFwd(Service* s, FsStorage* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 200;

    Result rc = serviceIpcDispatch(s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreate(&out->s, r.Handles[0]);
        }
    }

    return rc;
}

Result fsOpenDataStorageByCurrentProcessFromDomainFwd(Service* s, u32 *out_object_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeaderForDomain(&c, sizeof(*raw), s->object_id);

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 200;

    Result rc = serviceIpcDispatch(s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParseForDomain(&r);

        struct {
            u64 magic;
            u64 result;
            u32 object_id;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out_object_id = resp->object_id;
        }
    }

    return rc;
}

Result fsOpenDataStorageByDataId(Service* s, FsStorageId storage_id, u64 data_id, FsStorage* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        FsStorageId storage_id;
        u64 data_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 202;
    raw->storage_id = storage_id;
    raw->data_id = data_id;

    Result rc = serviceIpcDispatch(s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            serviceCreate(&out->s, r.Handles[0]);
        }
    }

    return rc;
}


Result fsOpenDataStorageByDataIdFromDomain(Service* s, FsStorageId storage_id, u64 data_id, u32 *out_object_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        FsStorageId storage_id;
        u64 data_id;
    } *raw;

    raw = ipcPrepareHeaderForDomain(&c, sizeof(*raw), s->object_id);

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 202;
    raw->storage_id = storage_id;
    raw->data_id = data_id;

    Result rc = serviceIpcDispatch(s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParseForDomain(&r);

        struct {
            u64 magic;
            u64 result;
            u32 object_id;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out_object_id = resp->object_id;
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

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->op_id = op_id;
    raw->off = off;
    raw->len = len;

    Result rc = serviceIpcDispatch(&f->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            FsRangeInfo range_info;
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && out) *out = resp->range_info;
    }

    return rc;
}

/* Missing FS Storage commands. */
Result fsStorageGetSize(FsStorage* s, u64* out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 4;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u64 size;
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && out) *out = resp->size;
    }

    return rc;
}

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

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 5;
    raw->op_id = op_id;
    raw->off = off;
    raw->len = len;

    Result rc = serviceIpcDispatch(&s->s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            FsRangeInfo range_info;
        } *resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc) && out) *out = resp->range_info;
    }

    return rc;
}