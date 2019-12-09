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

typedef struct {
    u64 keys_held;
    u64 flags;
} CfgOverrideStatus;

Result pminfoAtmosphereGetProcessId(u64 *out_pid, u64 program_id);
Result pminfoAtmosphereHasLaunchedProgram(bool *out, u64 program_id);
Result pminfoAtmosphereGetProcessInfo(NcmProgramLocation *loc_out, CfgOverrideStatus *status_out, u64 pid);

Result pmdmntAtmosphereGetProcessInfo(Handle *out, NcmProgramLocation *loc_out, CfgOverrideStatus *status_out, u64 pid);
Result pmdmntAtmosphereGetCurrentLimitInfo(u64 *out_cur, u64 *out_lim, u32 group, u32 resource);

#ifdef __cplusplus
}
#endif