/**
 * @file pm_ams.h
 * @brief Process Manager (pm:*) IPC wrapper for Atmosphere extensions.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

Result pminfoAtmosphereGetProcessId(u64 *out_pid, u64 tid);
Result pminfoAtmosphereHasLaunchedTitle(bool *out, u64 tid);

Result pmdmntAtmosphereGetProcessInfo(Handle *out, u64 *tid_out, u8 *sid_out, u64 pid);
Result pmdmntAtmosphereGetCurrentLimitInfo(u64 *out_cur, u64 *out_lim, u32 group, u32 resource);

#ifdef __cplusplus
}
#endif