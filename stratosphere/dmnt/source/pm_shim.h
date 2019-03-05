/**
 * @file pm_shim.h
 * @brief Process Management (pm) IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Atmosphere extension commands. */
Result pmdmntAtmosphereGetProcessInfo(Handle* out, u64 *tid_out, FsStorageId *sid_out, u64 pid);

#ifdef __cplusplus
}
#endif