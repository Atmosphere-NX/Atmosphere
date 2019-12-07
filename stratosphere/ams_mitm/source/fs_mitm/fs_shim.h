/**
 * @file fs_shim.h
 * @brief Filesystem Services (fs) IPC wrapper for fs.mitm.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Missing fsp-srv commands. */
Result fsOpenSdCardFileSystemFwd(Service* s, FsFileSystem* out);
Result fsOpenBisStorageFwd(Service* s, FsStorage* out, FsBisPartitionId partition_id);
Result fsOpenDataStorageByCurrentProcessFwd(Service* s, FsStorage* out);
Result fsOpenDataStorageByDataIdFwd(Service* s, FsStorage* out, u64 data_id, NcmStorageId storage_id);

Result fsOpenSaveDataFileSystemFwd(Service* s, FsFileSystem* out, FsSaveDataSpaceId save_data_space_id, const FsSaveDataAttribute *attr);


#ifdef __cplusplus
}
#endif