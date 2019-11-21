/**
 * @file smm_ams.h
 * @brief Service manager manager (sm:m) IPC wrapper for Atmosphere extensions.
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

Result smManagerAtmosphereEndInitialDefers(void);
Result smManagerAtmosphereRegisterProcess(u64 pid, u64 tid, const CfgOverrideStatus *status, const void *acid_sac, size_t acid_sac_size, const void *aci_sac, size_t aci_sac_size);
Result smManagerAtmosphereHasMitm(bool *out, SmServiceName name);

#ifdef __cplusplus
}
#endif