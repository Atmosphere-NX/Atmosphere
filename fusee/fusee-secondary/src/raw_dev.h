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
 
#ifndef FUSEE_RAW_DEV_H
#define FUSEE_RAW_DEV_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "device_partition.h"
#include "key_derivation.h"

#define RAWDEV_MAX_DEVICES 16

int rawdev_mount_device(const char *name, const device_partition_t *devpart, bool initialize_immediately);
int rawdev_register_device(const char *name);
int rawdev_unregister_device(const char *name);
int rawdev_unmount_device(const char *name);        /* also unregisters. */

int rawdev_register_keys(const char *name, unsigned int target_firmware, BisPartition part);

int rawdev_unmount_all(void);

#endif
