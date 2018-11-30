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
 
#ifndef EXOSPHERE_EXOSPHERE_CONFIG_H
#define EXOSPHERE_EXOSPHERE_CONFIG_H

#include <stdint.h>
#include "utils.h"

#include "memory_map.h"

/* This serves to set configuration for *exosphere itself*, separate from the SecMon Exosphere mimics. */

/* "XBC0" */
#define MAGIC_EXOSPHERE_BOOTCONFIG_0 (0x30434258)
/* "XBC1" */
#define MAGIC_EXOSPHERE_BOOTCONFIG (0x31434258)

#define EXOSPHERE_TARGET_FIRMWARE_100 1
#define EXOSPHERE_TARGET_FIRMWARE_200 2
#define EXOSPHERE_TARGET_FIRMWARE_300 3
#define EXOSPHERE_TARGET_FIRMWARE_400 4
#define EXOSPHERE_TARGET_FIRMWARE_500 5
#define EXOSPHERE_TARGET_FIRMWARE_600 6
#define EXOSPHERE_TARGET_FIRMWARE_620 7

#define EXOSPHERE_TARGET_FIRMWARE_CURRENT EXOSPHERE_TARGET_FIRMWARE_620

/* TODO: What should this be, for release? */
#define EXOSPHERE_TARGET_FIRMWARE_DEFAULT_FOR_DEBUG EXOSPHERE_TARGET_FIRMWARE_CURRENT
#define EXOSPHERE_LOOSEN_PACKAGE2_RESTRICTIONS_FOR_DEBUG 1

#define MAILBOX_BASE_PHYS  (MMIO_GET_DEVICE_PA(MMIO_DEVID_NXBOOTLOADER_MAILBOX))

/* TODO: Should this be at a non-static location? */
#define MAILBOX_EXOSPHERE_CONFIG_PHYS (*((volatile exosphere_config_t *)(MAILBOX_BASE_PHYS + 0xE40ULL)))

#define EXOSPHERE_FLAGS_DEFAULT 0x00000000
#define EXOSPHERE_FLAG_PERFORM_620_KEYGEN (1 << 0u)
#define EXOSPHERE_FLAG_IS_DEBUGMODE_PRIV  (1 << 1u)
#define EXOSPHERE_FLAG_IS_DEBUGMODE_USER  (1 << 2u)

typedef struct {
    unsigned int magic;
    unsigned int target_firmware;
    unsigned int flags;
} exosphere_config_t;

unsigned int exosphere_load_config(void);
unsigned int exosphere_get_target_firmware(void);
unsigned int exosphere_should_perform_620_keygen(void);
unsigned int exosphere_should_override_debugmode_priv(void);
unsigned int exosphere_should_override_debugmode_user(void);

static inline unsigned int exosphere_get_target_firmware_for_init(void) {
    const unsigned int magic = MAILBOX_EXOSPHERE_CONFIG_PHYS.magic;
    if (magic == MAGIC_EXOSPHERE_BOOTCONFIG || magic == MAGIC_EXOSPHERE_BOOTCONFIG_0) {
        return MAILBOX_EXOSPHERE_CONFIG_PHYS.target_firmware;
    } else {
        return EXOSPHERE_TARGET_FIRMWARE_DEFAULT_FOR_DEBUG;
    }
}

#endif
