/**
 * @file ns_shim.h
 * @brief Nintendo Shell Services (ns) IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Service s;
} NsDocumentInterface;

/* Command forwarders. */
Result nsGetDocumentInterfaceFwd(Service* s, NsDocumentInterface* out);

Result nsamGetApplicationContentPathFwd(Service* s, void* out, size_t out_size, u64 app_id, FsStorageId storage_id);
Result nsamResolveApplicationContentPathFwd(Service* s, u64 title_id, FsStorageId storage_id);
Result nsamGetRunningApplicationProgramIdFwd(Service* s, u64* out_tid, u64 app_id);

Result nswebGetApplicationContentPath(NsDocumentInterface* doc, void* out, size_t out_size, u64 app_id, FsStorageId storage_id);
Result nswebResolveApplicationContentPath(NsDocumentInterface* doc, u64 title_id, FsStorageId storage_id);
Result nswebGetRunningApplicationProgramId(NsDocumentInterface* doc, u64* out_tid, u64 app_id);

#ifdef __cplusplus
}
#endif