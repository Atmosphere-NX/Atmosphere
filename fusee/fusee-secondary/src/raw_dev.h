#ifndef FUSEE_RAW_DEV_H
#define FUSEE_RAW_DEV_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "device_partition.h"

#define RAWDEV_MAX_DEVICES 16

int rawdev_mount_device(const char *name, const device_partition_t *device, bool mount_immediately);
int rawdev_register_device(const char *name);

int rawdev_unregister_device(const char *name);
int rawdev_unmount_device(const char *name); /* also unregisters. */
int rawdev_unmount_all(void);

#endif
