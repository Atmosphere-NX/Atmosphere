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

#ifndef FUSEE_FS_DEV_H
#define FUSEE_FS_DEV_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "device_partition.h"
#include "key_derivation.h"

int fsdev_mount_device(const char *name, const device_partition_t *devpart, bool initialize_immediately);
int fsdev_register_device(const char *name);
int fsdev_unregister_device(const char *name);
int fsdev_unmount_device(const char *name);         /* also unregisters. */

int fsdev_register_keys(const char *name, unsigned int target_firmware, BisPartition part);

int fsdev_set_attr(const char *file, int attr, int mask);       /* Non-standard function to set file DOS attributes. */
int fsdev_get_attr(const char *file);                           /* Non-standard function to get file DOS attributes. */

int fsdev_set_default_device(const char *name);     /* must be registered. */
int fsdev_is_exfat(const char *name);               /* must be registered. */

int fsdev_unmount_all(void);

#endif
