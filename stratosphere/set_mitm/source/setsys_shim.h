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

typedef struct {
    char edid[0x100];
} SetSysEdid;

/* Command forwarders. */
Result setsysGetEdidFwd(Service* s, SetSysEdid* out);

#ifdef __cplusplus
}
#endif