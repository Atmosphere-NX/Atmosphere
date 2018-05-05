#ifndef FUSEE_FS_DEV_H
#define FUSEE_FS_DEV_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

int fsdev_mount_device(const char *name);
int fsdev_set_default_device(const char *name);
int fsdev_unmount_device(const char *name);

int fsdev_mount_all(void);
int fsdev_unmount_all(void);

#endif
