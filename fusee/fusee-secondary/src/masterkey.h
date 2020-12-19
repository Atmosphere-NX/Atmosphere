/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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

#ifndef FUSEE_MASTERKEY_H
#define FUSEE_MASTERKEY_H

/* This is glue code to enable master key support across versions. */

/* TODO: Update to 0xC on release of new master key. */
#define MASTERKEY_REVISION_MAX 0xB

#define MASTERKEY_REVISION_100_230     0x00
#define MASTERKEY_REVISION_300         0x01
#define MASTERKEY_REVISION_301_302     0x02
#define MASTERKEY_REVISION_400_410     0x03
#define MASTERKEY_REVISION_500_510     0x04
#define MASTERKEY_REVISION_600_610     0x05
#define MASTERKEY_REVISION_620         0x06
#define MASTERKEY_REVISION_700_800     0x07
#define MASTERKEY_REVISION_810         0x08
#define MASTERKEY_REVISION_900         0x09
#define MASTERKEY_REVISION_910_CURRENT 0x0A

#define MASTERKEY_NUM_NEW_DEVICE_KEYS (MASTERKEY_REVISION_MAX - MASTERKEY_REVISION_400_410)

/* This should be called early on in initialization. */
int mkey_detect_revision(bool is_retail);

unsigned int mkey_get_revision(void);
unsigned int mkey_get_keyslot(unsigned int revision);

void derive_new_device_keys(bool is_retail, unsigned int keygen_keyslot, unsigned int target_firmware);
void set_old_devkey(unsigned int revision, const uint8_t *key);
unsigned int devkey_get_keyslot(unsigned int revision);

#endif