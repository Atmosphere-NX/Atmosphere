/**
 * @file sfdnsres_shim.h
 * @brief IPC wrapper for dns.mitm.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Command forwarders. */
Result sfdnsresGetHostByNameRequestWithOptionsFwd(Service *s, u64 process_id, const void *name, size_t name_size, void *out_hostent, size_t out_hostent_size, u32 *out_size, u32 options_version, const void *option, size_t option_size, u32 num_options, s32 *out_host_error, s32 *out_errno);

Result sfdnsresGetAddrInfoRequestWithOptionsFwd(Service *s, u64 process_id, const void *node, size_t node_size, const void *srv, size_t srv_size, const void *hint, size_t hint_size, void *out_ai, size_t out_ai_size, u32 *out_size, s32 *out_rv, u32 options_version, const void *option, size_t option_size, u32 num_options, s32 *out_host_error, s32 *out_errno);

#ifdef __cplusplus
}
#endif