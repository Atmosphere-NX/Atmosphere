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
Result fsOpenBisStorageFwd(Service* s, FsStorage* out, FsBisPartitionId partition_id);

#ifdef __cplusplus
}
#endif