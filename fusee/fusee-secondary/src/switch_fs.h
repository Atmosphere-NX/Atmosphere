#ifndef FUSEE_SWITCH_FS_H
#define FUSEE_SWITCH_FS_H

#include "fs_dev.h"
#include "raw_dev.h"

int switchfs_mount_all(void);
int switchfs_unmount_all(void);

#endif
