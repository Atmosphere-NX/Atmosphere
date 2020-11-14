/**
 * @file sm_ams.h
 * @brief Service manager (sm) IPC wrapper for Atmosphere extensions.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

Result smAtmosphereHasService(bool *out, SmServiceName name);
Result smAtmosphereWaitService(SmServiceName name);
Result smAtmosphereHasMitm(bool *out, SmServiceName name);
Result smAtmosphereWaitMitm(SmServiceName name);

Result smAtmosphereMitmInitialize(void);
void smAtmosphereMitmExit(void);
Service *smAtmosphereMitmGetServiceSession();

Result smAtmosphereOpenSession(Service *out);
void smAtmosphereCloseSession(Service *srv);

Result smAtmosphereMitmInstall(Service *fwd_srv, Handle *handle_out, Handle *query_out, SmServiceName name);
Result smAtmosphereMitmUninstall(SmServiceName name);
Result smAtmosphereMitmDeclareFuture(SmServiceName name);
Result smAtmosphereMitmClearFuture(SmServiceName name);
Result smAtmosphereMitmAcknowledgeSession(Service *srv_out, void *info_out, SmServiceName name);

#ifdef __cplusplus
}
#endif