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

Result ldrPmAtmosphereHasLaunchedTitle(bool *out, u64 tid);
Result ldrDmntAtmosphereHasLaunchedTitle(bool *out, u64 tid);

#ifdef __cplusplus
}
#endif