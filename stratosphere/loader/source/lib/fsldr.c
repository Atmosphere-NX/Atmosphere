// Copyright 2018 SciresM
#include <string.h>
#include <switch/types.h>
#include <switch/result.h>
#include <switch/arm/atomics.h>
#include <switch/kernel/ipc.h>
#include <switch/kernel/detect.h>
#include <switch/services/fs.h>
#include <switch/services/sm.h>
#include "fsldr.h"

static Service g_fsldrSrv;
static u64 g_fsldrRefCnt;

Result fsldrSetCurrentProcess();

Result fsldrInitialize(void) {
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_fsldrSrv))
        return 0;

    Result rc = smGetService(&g_fsldrSrv, "fsp-ldr");

    if (R_SUCCEEDED(rc) && kernelAbove400()) {
        rc = fsldrSetCurrentProcess();
    }

    return rc;

}

void fsldrExit(void) {
    if (atomicDecrement64(&g_fsldrRefCnt) == 0)
        serviceClose(&g_fsldrSrv);
}

Result fsldrOpenCodeFileSystem(u64 tid, const char *path, FsFileSystem* out) {
    char send_path[FS_MAX_PATH] = {0};
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, send_path, FS_MAX_PATH, 0);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u64 tid;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 0;
    raw->tid = tid;
    
    strncpy(send_path, path, FS_MAX_PATH);
    Result rc = serviceIpcDispatch(&g_fsldrSrv);
    
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

Result fsldrIsArchivedProgram(u64 pid, bool *out) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
    } *raw;
    
    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->pid = pid;
    
    Result rc = serviceIpcDispatch(&g_fsldrSrv);
    
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
            u8 is_archived;
        } *resp = r.Raw;

        rc = resp->result;
        
        if (R_SUCCEEDED(rc)) {
            *out = is_archived != 0;
        }
    }
    
    return rc;
}

Result fsldrSetCurrentProcess() {
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendPid(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 unk;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 2;
    raw->unk = 0;

    rc = serviceIpcDispatch(&g_fsldrSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }
    
    return rc;
}

