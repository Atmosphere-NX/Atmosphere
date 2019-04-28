/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
 
#ifndef EXOSPHERE_CFG_ITEM_H
#define EXOSPHERE_CFG_ITEM_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CONFIGITEM_DISABLEPROGRAMVERIFICATION = 1,
    CONFIGITEM_DRAMID = 2,
    CONFIGITEM_SECURITYENGINEIRQ = 3,
    CONFIGITEM_VERSION = 4,
    CONFIGITEM_HARDWARETYPE = 5,
    CONFIGITEM_ISRETAIL = 6,
    CONFIGITEM_ISRECOVERYBOOT = 7,
    CONFIGITEM_DEVICEID = 8,
    CONFIGITEM_BOOTREASON = 9,
    CONFIGITEM_MEMORYARRANGE = 10,
    CONFIGITEM_ISDEBUGMODE = 11,
    CONFIGITEM_KERNELCONFIGURATION = 12,
    CONFIGITEM_HIZMODE = 13,
    CONFIGITEM_ISQUESTUNIT = 14,
    CONFIGITEM_NEWHARDWARETYPE_5X = 15,
    CONFIGITEM_NEWKEYGENERATION_5X = 16,
    CONFIGITEM_PACKAGE2HASH_5X = 17,
    
    /* These are unofficial, for usage by Exosphere. */
    CONFIGITEM_EXOSPHERE_VERSION = 65000,
    CONFIGITEM_NEEDS_REBOOT = 65001,
    CONFIGITEM_NEEDS_SHUTDOWN = 65002,
    CONFIGITEM_EXOSPHERE_VERHASH = 65003,
    CONFIGITEM_HAS_RCM_BUG_PATCH = 65004,
} ConfigItem;

#define REBOOT_KIND_NO_REBOOT     0
#define REBOOT_KIND_TO_RCM        1
#define REBOOT_KIND_TO_WB_PAYLOAD 2

uint32_t configitem_set(bool privileged, ConfigItem item, uint64_t value);
uint32_t configitem_get(bool privileged, ConfigItem item, uint64_t *p_outvalue);

bool configitem_is_recovery_boot(void);
bool configitem_is_retail(void);
bool configitem_is_hiz_mode_enabled(void);
bool configitem_is_debugmode_priv(void);

void configitem_set_debugmode_override(bool user, bool priv);
void configitem_set_hiz_mode_enabled(bool enabled);

uint64_t configitem_get_hardware_type(void);

#endif
