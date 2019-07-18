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

Result smManagerAtmosphereEndInitialDefers(void);
Result smManagerAtmosphereRegisterProcess(u64 pid, u64 tid, const void *acid_sac, size_t acid_sac_size, const void *aci_sac, size_t aci_sac_size);
Result smManagerAtmosphereHasMitm(bool *out, const char* name);

#ifdef __cplusplus
}
#endif