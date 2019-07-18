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

#ifdef __cplusplus
}
#endif