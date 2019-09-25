/**
 * @file setsys_shim.h
 * @brief System Settings Services (set:sys) IPC wrapper. To be merged into libnx, eventually.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Command forwarders. */
Result setsysGetSettingsItemValueFwd(Service* s, const char *name, const char *item_key, void *value_out, size_t value_out_size, u64 *size_out);

#ifdef __cplusplus
}
#endif