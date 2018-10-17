/**
 * @file smm_ams.h
 * @brief Service manager (sm:m) IPC wrapper for Atmosphere extensions.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

Result smManagerAmsInitialize(void);
void smManagerAmsExit(void);

Result smManagerAmsEndInitialDefers(void);

#ifdef __cplusplus
}
#endif