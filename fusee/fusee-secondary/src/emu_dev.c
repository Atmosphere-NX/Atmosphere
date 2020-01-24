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
 
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/iosupport.h>
#include <sys/param.h>
#include <unistd.h>

#include "emu_dev.h"
#include "utils.h"

static int       emudev_open(struct _reent *r, void *fileStruct, const char *path, int flags, int mode);
static int       emudev_close(struct _reent *r, void *fd);
static ssize_t   emudev_write(struct _reent *r, void *fd, const char *ptr, size_t len);
static ssize_t   emudev_read(struct _reent *r, void *fd, char *ptr, size_t len);
static off_t     emudev_seek(struct _reent *r, void *fd, off_t pos, int whence);
static int       emudev_fstat(struct _reent *r, void *fd, struct stat *st);
static int       emudev_stat(struct _reent *r, const char *file, struct stat *st);
static int       emudev_fsync(struct _reent *r, void *fd);

typedef struct emudev_device_t {
    devoptab_t devoptab;

    char origin_path[0x300+1];
    int num_parts;
    uint64_t part_limit;
    uint8_t *tmp_sector;
    device_partition_t devpart;
    char name[32+1];
    char root_path[34+1];
    bool setup, registered;
} emudev_device_t;

typedef struct emudev_file_t {
    emudev_device_t *device;
    int open_flags;
    uint64_t offset;
} emudev_file_t;

static emudev_device_t g_emudev_devices[EMUDEV_MAX_DEVICES] = {0};

static devoptab_t g_emudev_devoptab = {
  .structSize   = sizeof(emudev_file_t),
  .open_r       = emudev_open,
  .close_r      = emudev_close,
  .write_r      = emudev_write,
  .read_r       = emudev_read,
  .seek_r       = emudev_seek,
  .fstat_r      = emudev_fstat,
  .stat_r       = emudev_stat,
  .fsync_r      = emudev_fsync,
  .deviceData   = NULL,
};

static emudev_device_t *emudev_find_device(const char *name) {
    for (size_t i = 0; i < EMUDEV_MAX_DEVICES; i++) {
        if (g_emudev_devices[i].setup && strcmp(g_emudev_devices[i].name, name) == 0) {
            return &g_emudev_devices[i];
        }
    }

    return NULL;
}

int emudev_mount_device(const char *name, const device_partition_t *devpart, const char *origin_path, int num_parts, uint64_t part_limit) {
    emudev_device_t *device = NULL;

    if (name[0] == '\0' || devpart == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (strlen(name) > 32) {
        errno = ENAMETOOLONG;
        return -1;
    }
    if (emudev_find_device(name) != NULL) {
        errno = EEXIST; /* Device already exists */
        return -1;
    }

    /* Find an unused slot. */
    for (size_t i = 0; i < EMUDEV_MAX_DEVICES; i++) {
        if (!g_emudev_devices[i].setup) {
            device = &g_emudev_devices[i];
            break;
        }
    }
    if (device == NULL) {
        errno = ENOMEM;
        return -1;
    }

    memset(device, 0, sizeof(emudev_device_t));
    device->devoptab = g_emudev_devoptab;
    device->devpart = *devpart;
    strcpy(device->name, name);
    strcpy(device->root_path, name);
    strcat(device->root_path, ":/");
    
    /* Copy the file path for file mode. */
    if (devpart->emu_use_file)
        strcpy(device->origin_path, origin_path);
    
    device->num_parts = num_parts;
    device->part_limit = part_limit;
    
    device->devoptab.name = device->name;
    device->devoptab.deviceData = device;
    
    /* Initialize immediately. */
    int rc = device->devpart.initializer(&device->devpart);
    if (rc != 0) {
        errno = rc;
        return -1;
    }

    /* Allocate memory for our intermediate sector. */
    device->tmp_sector = (uint8_t *)malloc(devpart->sector_size);
    if (device->tmp_sector == NULL) {
        errno = ENOMEM;
        return -1;
    }

    device->setup = true;
    device->registered = false;

    return 0;
}

int emudev_register_device(const char *name) {
    emudev_device_t *device = emudev_find_device(name);
    if (device == NULL) {
        errno = ENOENT;
        return -1;
    }

    if (device->registered) {
        /* Do nothing if the device is already registered. */
        return 0;
    }

    if (AddDevice(&device->devoptab) == -1) {
        errno = ENOMEM;
        return -1;
    } else {
        device->registered = true;
        return 0;
    }
}

int emudev_unregister_device(const char *name) {
    emudev_device_t *device = emudev_find_device(name);
    char drname[40];

    if (device == NULL) {
        errno = ENOENT;
        return -1;
    }

    if (!device->registered) {
        /* Do nothing if the device is not registered. */
        return 0;
    }

    strcpy(drname, name);
    strcat(drname, ":");

    if (RemoveDevice(drname) == -1) {
        errno = ENOENT;
        return -1;
    } else {
        device->registered = false;
        return 0;
    }
}

int emudev_unmount_device(const char *name) {
    int rc;
    emudev_device_t *device = emudev_find_device(name);

    if (device == NULL) {
        errno = ENOENT;
        return -1;
    }

    rc = emudev_unregister_device(name);
    if (rc == -1) {
        return -1;
    }
    
    free(device->tmp_sector);
    device->devpart.finalizer(&device->devpart);
    memset(device, 0, sizeof(emudev_device_t));

    return 0;
}

int emudev_unmount_all(void) {
    for (size_t i = 0; i < EMUDEV_MAX_DEVICES; i++) {
        int rc = emudev_unmount_device(g_emudev_devices[i].name);
        if (rc != 0) {
            return rc;
        }
    }

    return 0;
}

static int emudev_open(struct _reent *r, void *fileStruct, const char *path, int flags, int mode) {
    (void)mode;
    emudev_file_t *f = (emudev_file_t *)fileStruct;
    emudev_device_t *device = (emudev_device_t *)(r->deviceData);

    /* Only allow "device:/". */
    if (strcmp(path, device->root_path) != 0) {
        r->_errno = ENOENT;
        return -1;
    }

    /* Forbid some flags that we explicitly don't support.*/
    if (flags & (O_APPEND | O_TRUNC | O_EXCL)) {
        r->_errno = EINVAL;
        return -1;
    }

    memset(f, 0, sizeof(emudev_file_t));
    f->device = device;
    f->open_flags = flags;
    return 0;
}

static int emudev_close(struct _reent *r, void *fd) {
    (void)r;
    emudev_file_t *f = (emudev_file_t *)fd;
    memset(f, 0, sizeof(emudev_file_t));

    return 0;
}

static ssize_t emudev_write(struct _reent *r, void *fd, const char *ptr, size_t len) {
    emudev_file_t *f = (emudev_file_t *)fd;
    emudev_device_t *device = f->device;
    size_t sector_size = device->devpart.sector_size;
    uint64_t sector_begin = f->offset / sector_size;
    uint64_t sector_end = (f->offset + len + sector_size - 1) / sector_size;
    uint64_t sector_end_aligned;
    uint64_t current_sector = sector_begin;
    const uint8_t *data = (const uint8_t *)ptr;

    int no = 0;

    if (sector_end >= device->devpart.num_sectors) {
        len = (size_t)(sector_size * device->devpart.num_sectors - f->offset);
        sector_end = device->devpart.num_sectors;
    }

    sector_end_aligned = sector_end - ((f->offset + len) % sector_size != 0 ? 1 : 0);

    if (len == 0) {
        return 0;
    }

    /* Unaligned at the start, we need to read the sector and incorporate the data. */
    if (f->offset % sector_size != 0) {
        size_t nb = (size_t)(len <= (sector_size - (f->offset % sector_size)) ? len : sector_size - (f->offset % sector_size));
        no = emu_device_partition_read_data(&device->devpart, device->tmp_sector, sector_begin, 1, device->origin_path, device->num_parts, device->part_limit);
        if (no != 0) {
            r->_errno = no;
            return -1;
        }
    
        memcpy(device->tmp_sector + (f->offset % sector_size), data, nb);

        no = emu_device_partition_write_data(&device->devpart, device->tmp_sector, sector_begin, 1, device->origin_path, device->num_parts, device->part_limit);
        if (no != 0) {
            r->_errno = no;
            return -1;
        }

        /* Advance */
        data += sector_size - (f->offset % sector_size);
        current_sector++;
    }

    /* Check if we're already done (otherwise this causes a bug in handling the last sector of the range). */
    if (current_sector == sector_end) {
        f->offset += len;
        return len;
    }

    /* Write all of the sector-aligned data. */
    if (current_sector != sector_end_aligned) {
        no = emu_device_partition_write_data(&device->devpart, data, current_sector, sector_end_aligned - current_sector, device->origin_path, device->num_parts, device->part_limit);
        if (no != 0) {
            r->_errno = no;
            return -1;
        }
    }

    data += sector_size * (sector_end_aligned - current_sector);
    current_sector = sector_end_aligned;

    /* Unaligned at the end, we need to read the sector and incorporate the data. */
    if (sector_end != sector_end_aligned) {        
        no = emu_device_partition_read_data(&device->devpart, device->tmp_sector, sector_end_aligned, 1, device->origin_path, device->num_parts, device->part_limit);
        if (no != 0) {
            r->_errno = no;
            return -1;
        }
    
        memcpy(device->tmp_sector, data, (size_t)((f->offset + len) % sector_size));
        
        no = emu_device_partition_write_data(&device->devpart, device->tmp_sector, sector_end_aligned, 1, device->origin_path, device->num_parts, device->part_limit);
        if (no != 0) {
            r->_errno = no;
            return -1;
        }

        /* Advance */
        data += sector_size - ((f->offset + len) % sector_size);
        current_sector++;
    }

    f->offset += len;
    return len;
}

static ssize_t emudev_read(struct _reent *r, void *fd, char *ptr, size_t len) {
    emudev_file_t *f = (emudev_file_t *)fd;
    emudev_device_t *device = f->device;
    size_t sector_size = device->devpart.sector_size;
    uint64_t sector_begin = f->offset / sector_size;
    uint64_t sector_end = (f->offset + len + sector_size - 1) / sector_size;
    uint64_t sector_end_aligned;
    uint64_t current_sector = sector_begin;
    uint8_t *data = (uint8_t *)ptr;

    int no = 0;

    if (sector_end >= device->devpart.num_sectors) {
        len = (size_t)(sector_size * device->devpart.num_sectors - f->offset);
        sector_end = device->devpart.num_sectors;
    }

    sector_end_aligned = sector_end - ((f->offset + len) % sector_size != 0 ? 1 : 0);

    if (len == 0) {
        return 0;
    }

    /* Unaligned at the start, we need to read the sector and incorporate the data. */
    if (f->offset % sector_size != 0) {
        size_t nb = (size_t)(len <= (sector_size - (f->offset % sector_size)) ? len : sector_size - (f->offset % sector_size));
        no = emu_device_partition_read_data(&device->devpart, device->tmp_sector, sector_begin, 1, device->origin_path, device->num_parts, device->part_limit);
        if (no != 0) {
            r->_errno = no;
            return -1;
        }
    
        memcpy(data, device->tmp_sector + (f->offset % sector_size), nb);

        /* Advance */
        data += sector_size - (f->offset % sector_size);
        current_sector++;
    }

    /* Check if we're already done (otherwise this causes a bug in handling the last sector of the range). */
    if (current_sector == sector_end) {
        f->offset += len;
        return len;
    }

    /* Read all of the sector-aligned data. */
    if (current_sector != sector_end_aligned) {        
        no = emu_device_partition_read_data(&device->devpart, data, current_sector, sector_end_aligned - current_sector, device->origin_path, device->num_parts, device->part_limit);
        if (no != 0) {
            r->_errno = no;
            return -1;
        }
    }

    data += sector_size * (sector_end_aligned - current_sector);
    current_sector = sector_end_aligned;

    /* Unaligned at the end, we need to read the sector and incorporate the data. */
    if (sector_end != sector_end_aligned) {
        no = emu_device_partition_read_data(&device->devpart, device->tmp_sector, sector_end_aligned, 1, device->origin_path, device->num_parts, device->part_limit);
        if (no != 0) {
            r->_errno = no;
            return -1;
        }

        memcpy(data, device->tmp_sector, (size_t)((f->offset + len) % sector_size));

        /* Advance */
        data += sector_size - ((f->offset + len) % sector_size);
        current_sector++;
    }

    f->offset += len;
    return len;
}

static off_t emudev_seek(struct _reent *r, void *fd, off_t pos, int whence) {
    emudev_file_t *f = (emudev_file_t *)fd;
    emudev_device_t *device = f->device;
    uint64_t off;

    switch (whence) {
        case SEEK_SET:
            off = 0;
            break;
        case SEEK_CUR:
            off = f->offset;
            break;
        case SEEK_END:
            off = device->devpart.num_sectors * device->devpart.sector_size;
            break;
        default:
            r->_errno = EINVAL;
            return -1;
    }

    if (pos < 0 && pos + off < 0) {
        /* don't allow seek to before the beginning of the file */
        r->_errno = EINVAL;
        return -1;
    }

    f->offset = (uint64_t)(pos + off);
    return (off_t)(pos + off);
}

static void emudev_stat_impl(emudev_device_t *device, struct stat *st) {
    memset(st, 0, sizeof(struct stat));
    st->st_size = (off_t)(device->devpart.num_sectors * device->devpart.sector_size);
    st->st_nlink = 1;

    st->st_blksize = device->devpart.sector_size;
    st->st_blocks = st->st_size / st->st_blksize;

    st->st_mode = S_IFBLK | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
}

static int emudev_fstat(struct _reent *r, void *fd, struct stat *st) {
    (void)r;
    emudev_file_t *f = (emudev_file_t *)fd;
    emudev_device_t *device = f->device;
    emudev_stat_impl(device, st);

    return 0;
}

static int emudev_stat(struct _reent *r, const char *file, struct stat *st) {
    emudev_device_t *device = (emudev_device_t *)(r->deviceData);
    if (strcmp(file, device->root_path) != 0) {
        r->_errno = ENOENT;
        return -1;
    }

    emudev_stat_impl(device, st);
    return 0;
}

static int emudev_fsync(struct _reent *r, void *fd) {
    /* Nothing to do. */
    (void)r;
    (void)fd;
    return 0;
}
