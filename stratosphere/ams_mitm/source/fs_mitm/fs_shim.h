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
Result fsOpenDataStorageWithProgramIndexFwd(Service* s, FsStorage* out, u8 program_index);

Result fsRegisterProgramIndexMapInfoFwd(Service* s, const void *buf, size_t buf_size, s32 count);

Result fsOpenSaveDataFileSystemFwd(Service* s, FsFileSystem* out, FsSaveDataSpaceId save_data_space_id, const FsSaveDataAttribute *attr);

Result fsOpenFileSystemWithPatchFwd(Service* s, FsFileSystem* out, u64 id, FsFileSystemType fsType);
Result fsOpenFileSystemWithIdFwd(Service* s, FsFileSystem* out, u64 id, FsFileSystemType fsType, const char* contentPath);


#ifdef __cplusplus
}
#endif