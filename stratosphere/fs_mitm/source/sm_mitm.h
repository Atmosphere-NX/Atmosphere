/**
 * @file sm_mitm.h
 * @brief Service manager (sm) IPC wrapper for Atmosphere extensions.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

Result smMitMInitialize(void);
void smMitMExit(void);
Result smMitMGetService(Service* service_out, const char *name);
Result smMitMInstall(Handle *handle_out, Handle *query_out, const char *name);
Result smMitMUninstall(const char *name);

Result smMitMIsRegistered(const char *name);

#ifdef __cplusplus
}
#endif