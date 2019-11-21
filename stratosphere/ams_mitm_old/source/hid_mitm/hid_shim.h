/**
 * @file hid_shim.h
 * @brief Human Interface Devices Services (hid) IPC wrapper.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Command forwarders. */
Result hidSetSupportedNpadStyleSetFwd(Service* s, u64 process_id, u64 aruid, HidControllerType type);

#ifdef __cplusplus
}
#endif