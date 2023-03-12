/**
 * @file setsys_shim.h
 * @brief Settings Services (fs) IPC wrapper for setsys.mitm.
 * @author ndeadly
 * @copyright libnx Authors
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forwarding shims. */
Result setsysSetBluetoothDevicesSettingsFwd(Service *s, const SetSysBluetoothDevicesSettings *settings, s32 count);
Result setsysGetBluetoothDevicesSettingsFwd(Service *s, s32 *total_out, SetSysBluetoothDevicesSettings *settings, s32 count);

#ifdef __cplusplus
}
#endif