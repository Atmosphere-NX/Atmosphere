#include <switch.h>
#include <string.h>

#include "ldr_registration.hpp"
#include "ldr_location_resolver.hpp"

Result GetContentPath(char *out_path, u64 tid, FsStorageId sid) {
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

Result GetContentPathForTidSid(char *out_path, Registration::TidSid *tid_sid) {
    return GetContentPath(out_path, tid_sid->title_id, tid_sid->storage_id);
}