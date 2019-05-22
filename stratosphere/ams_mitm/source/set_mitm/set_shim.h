/**
 * @file set_shim.h
 * @brief Settings Services (set) IPC wrapper. To be merged into libnx, eventually.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Command forwarders. */
Result setGetAvailableLanguageCodesFwd(Service* s, s32 *total_entries, u64 *language_codes, size_t max_entries);

#ifdef __cplusplus
}
#endif