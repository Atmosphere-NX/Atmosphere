/*
 * Copyright (c) 2018 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <stdint.h>
#include <stdbool.h>

#include "utils.h"
#include "exocfg.h"
#include "mmu.h"
#include "memory_map.h"

#define MAILBOX_BASE  (MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_NXBOOTLOADER_MAILBOX))

/* TODO: Should this be at a non-static location? */
#define MAILBOX_EXOSPHERE_CONFIG (*((volatile exosphere_config_t *)(MAILBOX_BASE + 0xE40ULL)))

static exosphere_config_t g_exosphere_cfg = {MAGIC_EXOSPHERE_BOOTCONFIG, EXOSPHERE_TARGET_FIRMWARE_DEFAULT_FOR_DEBUG, EXOSPHERE_FLAGS_DEFAULT};
static bool g_has_loaded_config = false;

#define EXOSPHERE_CHECK_FLAG(flag) ((g_exosphere_cfg.flags & flag) != 0)


/* Read config out of IRAM, return target firmware version. */
unsigned int exosphere_load_config(void) {
    if (g_has_loaded_config) {
        generic_panic();
    }
    g_has_loaded_config = true;
    
    const unsigned int magic = MAILBOX_EXOSPHERE_CONFIG.magic;
    
    if (magic == MAGIC_EXOSPHERE_BOOTCONFIG) {
        g_exosphere_cfg = MAILBOX_EXOSPHERE_CONFIG;
    } else if (magic == MAGIC_EXOSPHERE_BOOTCONFIG_0) {
        g_exosphere_cfg = MAILBOX_EXOSPHERE_CONFIG;
        g_exosphere_cfg.flags = EXOSPHERE_FLAGS_DEFAULT;
    }
    
    return g_exosphere_cfg.target_firmware;
}

unsigned int exosphere_get_target_firmware(void) {
    if (!g_has_loaded_config) {
        generic_panic();
    }
    
    return g_exosphere_cfg.target_firmware;
}

unsigned int exosphere_should_perform_620_keygen(void) {
    if (!g_has_loaded_config) {
        generic_panic();
    }
    
    return g_exosphere_cfg.target_firmware >= EXOSPHERE_TARGET_FIRMWARE_620 && EXOSPHERE_CHECK_FLAG(EXOSPHERE_FLAG_PERFORM_620_KEYGEN);
}

unsigned int exosphere_should_override_debugmode_priv(void) {
    if (!g_has_loaded_config) {
        generic_panic();
    }
    
    return EXOSPHERE_CHECK_FLAG(EXOSPHERE_FLAG_IS_DEBUGMODE_PRIV);
}

unsigned int exosphere_should_override_debugmode_user(void) {
    if (!g_has_loaded_config) {
        generic_panic();
    }
    
    return EXOSPHERE_CHECK_FLAG(EXOSPHERE_FLAG_IS_DEBUGMODE_USER);
}
