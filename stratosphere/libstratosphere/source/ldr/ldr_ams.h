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

Result ldrPmAtmosphereHasLaunchedProgram(bool *out, u64 program_id);
Result ldrDmntAtmosphereHasLaunchedProgram(bool *out, u64 program_id);

#ifdef __cplusplus
}
#endif