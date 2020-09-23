/**
 * @file set_shim.h
 * @brief Settings Services (fs) IPC wrapper for set.mitm.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forwarding shims. */
Result setGetLanguageCodeFwd(Service *s, u64* out);
Result setGetRegionCodeFwd(Service *s, SetRegion *out);

#ifdef __cplusplus
}
#endif