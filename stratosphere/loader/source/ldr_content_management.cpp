#include <switch.h>
#include <string.h>

#include "ldr_registration.hpp"
#include "ldr_content_management.hpp"

static FsFileSystem g_CodeFileSystem = {0};

Result ContentManagement::MountCode(u64 tid, FsStorageId sid) {
    char path[FS_MAX_PATH] = {0};
    Result rc;
    
    if (R_FAILED(rc = GetContentPath(path, tid, sid))) {
        return rc;
    }
    
    /* Always re-initialize fsp-ldr, in case it's closed */
    if (R_FAILED(rc = fsldrInitialize())) {
        return rc;
    }
    
    if (R_FAILED(rc = fsldrOpenCodeFileSystem(tid, path, &g_CodeFileSystem))) {
        fsldrExit();
        return rc;
    }
    
    fsdevMountDevice("code", g_CodeFileSystem);
    
    return rc;
}

Result ContentManagement::UnmountCode() {
    fsdevUnmountDevice("code");
    serviceClose(&g_CodeFileSystem.s);
    return 0;
}

Result ContentManagement::MountCodeForTidSid(Registration::TidSid *tid_sid) {
    return MountCode(tid_sid->title_id, tid_sid->storage_id);
}

Result ContentManagement::GetContentPath(char *out_path, u64 tid, FsStorageId sid) {
    Result rc;
    LrRegisteredLocationResolver reg;
    LrLocationResolver lr;
    char path[FS_MAX_PATH] = {0};
    
    /* Try to get the path from the registered resolver. */
    if (R_FAILED(rc = lrGetRegisteredLocationResolver(&reg))) {
        return rc;
    }
    
    if (R_SUCCEEDED(rc = lrRegLrGetProgramPath(&reg, tid, path))) {
        strncpy(out_path, path, sizeof(path));
    } else if (rc != 0x408) {
        return rc;
    }
    
    serviceClose(&reg.s);
    
    /* If getting the path from the registered resolver fails, fall back to the normal resolver. */
    if (R_FAILED(rc = lrGetLocationResolver(sid, &lr))) {
        return rc;
    }
    
    if (R_SUCCEEDED(rc = lrLrGetProgramPath(&lr, tid, path))) {
        strncpy(out_path, path, sizeof(path));
    }
    
    serviceClose(&lr.s);
    
    return rc;
}

Result ContentManagement::GetContentPathForTidSid(char *out_path, Registration::TidSid *tid_sid) {
    return GetContentPath(out_path, tid_sid->title_id, tid_sid->storage_id);
}

Result ContentManagement::SetContentPath(char *path, u64 tid, FsStorageId sid) {
    Result rc;
    LrLocationResolver lr;
    
    if (R_FAILED(rc = lrGetLocationResolver(sid, &lr))) {
        return rc;
    }
    
    rc = lrLrSetProgramPath(&lr, tid, path);
    
    serviceClose(&lr.s);
    
    return rc;
}

Result ContentManagement::SetContentPathForTidSid(char *path, Registration::TidSid *tid_sid) {
    return SetContentPath(path, tid_sid->title_id, tid_sid->storage_id);
}