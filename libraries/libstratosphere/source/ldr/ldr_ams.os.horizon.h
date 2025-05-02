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

Result ldrPmAtmosphereHasLaunchedBootProgram(bool *out, u64 program_id);
Result ldrDmntAtmosphereHasLaunchedBootProgram(bool *out, u64 program_id);

Result ldrPmAtmosphereGetProgramInfo(LoaderProgramInfo *out, CfgOverrideStatus *out_status, const NcmProgramLocation *loc, const LoaderProgramAttributes *attr);
Result ldrPmAtmospherePinProgram(u64 *out, const NcmProgramLocation *loc, const CfgOverrideStatus *status);

#ifdef __cplusplus
}
#endif