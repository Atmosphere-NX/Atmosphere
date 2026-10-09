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
Result fsOpenFileSystemWithPatchFwd(Service* s, FsFileSystem* out, u64 id, FsFileSystemType fsType);
Result fsOpenFileSystemWithIdObsoleteFwd(Service* s, FsFileSystem* out, const char* contentPath, u64 id, FsFileSystemType fsType);
Result fsOpenFileSystemWithIdFwd(Service* s, FsFileSystem* out, const char* contentPath, FsContentAttributes attributes, u64 id, FsFileSystemType fsType);

Result fsOpenBisStorageFwd(Service* s, FsStorage* out, FsBisPartitionId partition_id);
Result fsOpenSdCardFileSystemFwd(Service* s, FsFileSystem* out);
Result fsOpenSaveDataFileSystemFwd(Service* s, FsFileSystem* out, FsSaveDataSpaceId save_data_space_id, const FsSaveDataAttribute *attr);

Result fsOpenDataStorageByCurrentProcessFwd(Service* s, FsStorage* out);
Result fsOpenDataStorageByProgramIdFwd(Service* s, FsStorage* out, u64 id);
Result fsOpenDataStorageByDataIdFwd(Service* s, FsStorage* out, u64 data_id, NcmStorageId storage_id);
Result fsOpenDataStorageWithProgramIndexFwd(Service* s, FsStorage* out, u8 program_index);
Result fsOpenDataStorageByPathFwd(Service* s, FsStorage* out, const char* contentPath, FsContentAttributes attributes, FsFileSystemType fsType);

Result fsOpenDataStorageByCurrentProcessForBatchReadFwd(Service* s, FsStorageForBatchRead* out);
Result fsOpenDataStorageByProgramIdForBatchReadFwd(Service* s, FsStorageForBatchRead* out, u64 id);
Result fsOpenDataStorageWithProgramIndexForBatchReadFwd(Service* s, FsStorageForBatchRead* out, u8 program_index);
Result fsOpenDataStorageByPathForBatchReadFwd(Service* s, FsStorageForBatchRead* out, const char* contentPath, FsContentAttributes attributes, FsFileSystemType fsType);

Result fsRegisterProgramIndexMapInfoFwd(Service* s, const void *buf, size_t buf_size, s32 count);


#ifdef __cplusplus
}
#endif