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
 
#ifndef FUSEE_EXOSPHERE_CONFIG_H
#define FUSEE_EXOSPHERE_CONFIG_H

/* This serves to set configuration for *exosphere itself*, separate from the SecMon Exosphere mimics. */

/* "XBC0" */
#define MAGIC_EXOSPHERE_BOOTCONFIG_0 (0x30434258)
/* "XBC1" */
#define MAGIC_EXOSPHERE_BOOTCONFIG   (0x31434258)

#define EXOSPHERE_TARGET_FIRMWARE_100 1
#define EXOSPHERE_TARGET_FIRMWARE_200 2
#define EXOSPHERE_TARGET_FIRMWARE_300 3
#define EXOSPHERE_TARGET_FIRMWARE_400 4
#define EXOSPHERE_TARGET_FIRMWARE_500 5
#define EXOSPHERE_TARGET_FIRMWARE_600 6
#define EXOSPHERE_TARGET_FIRMWARE_620 7

#define EXOSPHERE_TARGET_FIRMWARE_MIN EXOSPHERE_TARGET_FIRMWARE_100
#define EXOSPHERE_TARGET_FIRMWARE_MAX EXOSPHERE_TARGET_FIRMWARE_620

#define EXOSPHERE_FLAGS_DEFAULT 0x00000000
#define EXOSPHERE_FLAG_PERFORM_620_KEYGEN (1 << 0u)
#define EXOSPHERE_FLAG_IS_DEBUGMODE_PRIV  (1 << 1u)
#define EXOSPHERE_FLAG_IS_DEBUGMODE_USER  (1 << 2u)

typedef struct {
    unsigned int magic;
    unsigned int target_firmware;
    unsigned int flags;
} exosphere_config_t;

#define MAILBOX_EXOSPHERE_CONFIGURATION ((volatile exosphere_config_t *)(0x40002E40))

#define EXOSPHERE_TARGETFW_KEY "target_firmware"
#define EXOSPHERE_DEBUGMODE_PRIV_KEY "debugmode"
#define EXOSPHERE_DEBUGMODE_USER_KEY "debugmode_user"

#endif