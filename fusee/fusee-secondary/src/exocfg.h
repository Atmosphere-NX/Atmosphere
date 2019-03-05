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

#include <atmosphere.h>

/* This serves to set configuration for *exosphere itself*, separate from the SecMon Exosphere mimics. */

/* "EXO0" */
#define MAGIC_EXOSPHERE_CONFIG (0x304F5845)

#define EXOSPHERE_FLAGS_DEFAULT 0x00000000
#define EXOSPHERE_FLAG_PERFORM_620_KEYGEN (1 << 0u)
#define EXOSPHERE_FLAG_IS_DEBUGMODE_PRIV  (1 << 1u)
#define EXOSPHERE_FLAG_IS_DEBUGMODE_USER  (1 << 2u)

typedef struct {
    unsigned int magic;
    unsigned int target_firmware;
    unsigned int flags;
    unsigned int reserved;
} exosphere_config_t;

#define MAILBOX_EXOSPHERE_CONFIGURATION ((volatile exosphere_config_t *)(0x8000F000ull))

#define EXOSPHERE_TARGETFW_KEY "target_firmware"
#define EXOSPHERE_DEBUGMODE_PRIV_KEY "debugmode"
#define EXOSPHERE_DEBUGMODE_USER_KEY "debugmode_user"

#endif