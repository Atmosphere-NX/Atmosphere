/**
 * @file fs_shim.h
 * @brief Filesystem Services (fs) IPC wrapper. To be merged into libnx, eventually.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TODO: Reverse this more. */
typedef struct {
    u32 flags[0x40/sizeof(u32)];
} FsRangeInfo;

/* Missing fsp-srv commands. */
Result fsOpenBisStorageFwd(Service* s, FsStorage* out, u32 PartitionId);
Result fsOpenDataStorageByCurrentProcessFwd(Service* s, FsStorage* out);
Result fsOpenDataStorageByDataIdFwd(Service* s, FsStorageId storage_id, u64 data_id, FsStorage* out);

/* Missing FS File commands. */
Result fsFileOperateRange(FsFile* f, u32 op_id, u64 off, u64 len, FsRangeInfo *out);

/* Missing FS Storage commands. */
Result fsStorageOperateRange(FsStorage* s, u32 op_id, u64 off, u64 len, FsRangeInfo *out);

#ifdef __cplusplus
}
#endif