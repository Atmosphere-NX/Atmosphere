/**
 * @file ldr_ams.h
 * @brief Loader (ldr:*) IPC wrapper for Atmosphere extensions.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u64 keys_down;
    u64 flags;
} CfgOverrideStatus;

Result ldrPmAtmosphereHasLaunchedProgram(bool *out, u64 program_id);
Result ldrDmntAtmosphereHasLaunchedProgram(bool *out, u64 program_id);

Result ldrPmAtmosphereGetProgramInfo(LoaderProgramInfo *out, CfgOverrideStatus *out_status, const NcmProgramLocation *loc);
Result ldrPmAtmospherePinProgram(u64 *out, const NcmProgramLocation *loc, const CfgOverrideStatus *status);

#ifdef __cplusplus
}
#endif