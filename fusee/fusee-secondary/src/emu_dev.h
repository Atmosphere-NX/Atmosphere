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
 
#ifndef FUSEE_EMU_DEV_H
#define FUSEE_EMU_DEV_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "device_partition.h"

#define EMUDEV_MAX_DEVICES 16

int emudev_mount_device(const char *name, const char *origin_path, const device_partition_t *devpart);
int emudev_register_device(const char *name);

int emudev_unregister_device(const char *name);
int emudev_unmount_device(const char *name); /* also unregisters. */
int emudev_unmount_all(void);

#endif
