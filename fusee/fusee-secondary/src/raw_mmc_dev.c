#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/iosupport.h>
#include <sys/param.h>
#include <unistd.h>

#include "raw_mmc_dev.h"

static int       rawmmcdev_open(struct _reent *r, void *fileStruct, const char *path, int flags, int mode);
static int       rawmmcdev_close(struct _reent *r, void *fd);
static ssize_t   rawmmcdev_write(struct _reent *r, void *fd, const char *ptr, size_t len);
static ssize_t   rawmmcdev_read(struct _reent *r, void *fd, char *ptr, size_t len);
static off_t     rawmmcdev_seek(struct _reent *r, void *fd, off_t pos, int whence);
static int       rawmmcdev_fstat(struct _reent *r, void *fd, struct stat *st);
static int       rawmmcdev_stat(struct _reent *r, const char *file, struct stat *st);
static int       rawmmcdev_fsync(struct _reent *r, void *fd);

typedef struct rawmmcdev_device_t {
    devoptab_t devoptab;

    struct mmc *mmc;
    enum sdmmc_partition partition;
    size_t offset;
    size_t size;
    rawmmc_crypt_func_t read_crypt_func;
    rawmmc_crypt_func_t write_crypt_func;
    uint64_t crypt_flags;
    uint8_t iv[16];

    char name[32+1];
    char root_path[34+1];
    bool setup, encrypted;
} rawmmcdev_device_t;

typedef struct rawmmcdev_file_t {
    rawmmcdev_device_t *device;
    int open_flags;
    size_t offset;
} rawmmcdev_file_t;

static rawmmcdev_device_t g_rawmmcdev_devices[RAWMMC_MAX_DEVICES] = {0};

static devoptab_t g_rawmmcdev_devoptab = {
  .structSize   = sizeof(rawmmcdev_file_t),
  .open_r       = rawmmcdev_open,
  .close_r      = rawmmcdev_close,
  .write_r      = rawmmcdev_write,
  .read_r       = rawmmcdev_read,
  .seek_r       = rawmmcdev_seek,
  .fstat_r      = rawmmcdev_fstat,
  .stat_r       = rawmmcdev_stat,
  .fsync_r      = rawmmcdev_fsync,
  .deviceData   = NULL,
};

int rawmmcdev_mount_device(
    const char *name,
    struct mmc *mmc,
    enum sdmmc_partition partition,
    size_t offset,
    size_t size,
    rawmmc_crypt_func_t read_crypt_func,
    rawmmc_crypt_func_t write_crypt_func,
    uint64_t crypt_flags,
    const uint8_t *iv
) {
    rawmmcdev_device_t *device = NULL;
    char drname[40];
    strcpy(drname, name);
    strcat(drname, ":");

    if (name[0] == '\0') {
        errno = EINVAL;
        return -1;
    }
    if ((read_crypt_func == NULL && write_crypt_func != NULL) || (read_crypt_func != NULL && write_crypt_func == NULL)) {
        errno = EINVAL;
        return -1;
    }
    if((offset % 512) != 0 || (size % 512) != 0) {
        errno = EINVAL;
        return -1;
    }
    if (strlen(name) > 32) {
        errno = ENAMETOOLONG;
        return -1;
    }
    if (FindDevice(drname) != -1) {
        errno = EEXIST; /* Device already exists */
        return -1;
    }

    /* Find an unused slot. */
    for(size_t i = 0; i < RAWMMC_MAX_DEVICES; i++) {
        if (!g_rawmmcdev_devices[i].setup) {
            device = &g_rawmmcdev_devices[i];
        }
    }
    if (device == NULL) {
        errno = ENOMEM;
        return -1;
    }

    memset(device, 0, sizeof(rawmmcdev_device_t));
    memcpy(&device->devoptab, &g_rawmmcdev_devoptab, sizeof(devoptab_t));
    strcpy(device->name, name);
    strcpy(device->root_path, name);
    strcat(device->root_path, ":/");
    device->mmc = mmc;
    device->partition = partition;
    device->offset = offset;
    device->size = size;
    device->read_crypt_func = read_crypt_func;
    device->write_crypt_func = write_crypt_func;
    device->encrypted = read_crypt_func != NULL || write_crypt_func != NULL;
    device->crypt_flags = crypt_flags;
    if (iv != NULL) {
        memcpy(device->iv, iv, 16);
    }

    device->devoptab.name = device->name;
    device->devoptab.deviceData = device;

    if (AddDevice(&device->devoptab) == -1) {
        errno = ENOMEM;
        return -1;
    } else {
        device->setup = true;
        return 0;
    }
}

int rawmmcdev_mount_unencrypted_device(
    const char *name,
    struct mmc *mmc,
    enum sdmmc_partition partition,
    size_t offset,
    size_t size
) {
    return rawmmcdev_mount_device(name, mmc, partition, offset, size, NULL, NULL, 0, NULL);
}

int rawmmcdev_unmount_device(const char *name) {
    char drname[40];
    int devid;
    rawmmcdev_device_t *device;

    strcpy(drname, name);
    strcat(drname, ":");

    devid = FindDevice(drname);

    if (devid == -1) {
        errno = ENOENT;
        return -1;
    }

    device = (rawmmcdev_device_t *)(GetDeviceOpTab(name)->deviceData);
    RemoveDevice(drname);
    memset(device, 0, sizeof(rawmmcdev_device_t));

    return 0;
}

int rawmmcdev_unmount_all(void) {
    for (size_t i = 0; i < RAWMMC_MAX_DEVICES; i++) {
        RemoveDevice(g_rawmmcdev_devices[i].root_path);
        memset(&g_rawmmcdev_devices[i], 0, sizeof(rawmmcdev_device_t));
    }

    return 0;
}

static int rawmmcdev_open(struct _reent *r, void *fileStruct, const char *path, int flags, int mode) {
    (void)mode;
    rawmmcdev_file_t *f = (rawmmcdev_file_t *)fileStruct;
    rawmmcdev_device_t *device = (rawmmcdev_device_t *)(r->deviceData);

    /* Only allow "device:/". */
    if (strcmp(path, device->root_path) != 0) {
        r->_errno = ENOENT;
        return -1;
    }

    /* Forbid some flags that we explicitely don't support.*/
    if (flags & (O_APPEND | O_TRUNC | O_EXCL)) {
        r->_errno = EINVAL;
        return -1;
    }

    memset(f, 0, sizeof(rawmmcdev_file_t));
    f->device = device;
    f->open_flags = flags;
    return 0;
}

static int rawmmcdev_close(struct _reent *r, void *fd) {
    (void)r;
    rawmmcdev_file_t *f = (rawmmcdev_file_t *)fd;
    memset(f, 0, sizeof(rawmmcdev_file_t));

    return 0;
}

/* Keep this <= the size of the DMA bounce buffer in sdmmc.c */
static __attribute__((aligned(16))) uint8_t g_crypto_buffer[512] = {0};

static ssize_t rawmmcdev_write(struct _reent *r, void *fd, const char *ptr, size_t len) {
    rawmmcdev_file_t *f = (rawmmcdev_file_t *)fd;
    rawmmcdev_device_t *device = f->device;
    uint32_t device_sector_offset = (uint32_t)(device->offset / 512);
    uint32_t sector_begin = (uint32_t)((device->offset + f->offset) / 512); /* NAND offset */
    uint32_t sector_end = (uint32_t)((device->offset + f->offset + len + 511) / 512);
    uint32_t sector_end_aligned = sector_end - ((f->offset + len) % 512 != 0 ? 1 : 0);
    uint32_t current_sector = sector_begin;
    const uint8_t *data = (const uint8_t *)ptr;

    int no = 0;

    if (f->offset + len >= device->offset + device->size) {
        len = device->size - f->offset;
    }

    /* Change the partition, if needed. */
    no = sdmmc_select_partition(device->mmc, device->partition);
    if (no != 0 && no != ENOTTY) {
        r->_errno = no;
        return -1;
    }

    /* Unaligned at the start, we need to read the sector and incorporate the data. */
    if (f->offset % 512 != 0) {
        no = sdmmc_read(device->mmc, g_crypto_buffer, sector_begin, 1);
        if (no != 0) {
            r->_errno = no;
            return -1;
        }
        if (device->encrypted) {
            device->read_crypt_func(g_crypto_buffer, g_crypto_buffer, 512 * (sector_begin - device_sector_offset), 512, device->iv, device->crypt_flags);
        }
        memcpy(g_crypto_buffer, data, len <= (512 - (f->offset % 512)) ? len : 512 - (f->offset % 512));
        if (device->encrypted) {
            device->write_crypt_func(g_crypto_buffer, g_crypto_buffer, 512 * (sector_begin - device_sector_offset), 512, device->iv, device->crypt_flags);
        }
        no = sdmmc_write(device->mmc, g_crypto_buffer, sector_begin, 1);
        if (no != 0) {
            r->_errno = no;
            return -1;
        }

        /* Advance */
        data += 512 - (f->offset % 512);
        current_sector++;
    }

    /* Check if we're already done (otherwise this causes a bug in handling the last sector of the range). */
    if (current_sector == sector_end) {
        f->offset += len;
        return len;
    }

    size_t sectors_remaining = sector_end_aligned - current_sector;
    for (size_t i = 0; i < len; i += sizeof(g_crypto_buffer)/512) {
        size_t n = sectors_remaining <= sizeof(g_crypto_buffer)/512 ? sectors_remaining : sizeof(g_crypto_buffer)/512;

        if (device->encrypted) {
            memcpy(g_crypto_buffer, data, 512 * n);
            device->write_crypt_func(g_crypto_buffer, g_crypto_buffer, current_sector - device_sector_offset, 512 * n, device->iv, device->crypt_flags);
            no = sdmmc_write(device->mmc, g_crypto_buffer, current_sector, n);
        } else {
            no = sdmmc_write(device->mmc, data, current_sector, n);
        }

        if (no != 0) {
            r->_errno = no;
            return -1;
        }

        data += 512 * n;
        current_sector += n;
    }

    /* Unaligned at the end, we need to read the sector and incorporate the data. */
    if (sector_end != sector_end_aligned) {
        no = sdmmc_read(device->mmc, g_crypto_buffer, sector_end_aligned, 1);
        if (no != 0) {
            r->_errno = no;
            return -1;
        }
        if (device->encrypted) {
            device->read_crypt_func(g_crypto_buffer, g_crypto_buffer, 512 * (sector_end_aligned - device_sector_offset), 512, device->iv, device->crypt_flags);
        }
        memcpy(g_crypto_buffer, data, (f->offset + len) % 512);
        if (device->encrypted) {
            device->write_crypt_func(g_crypto_buffer, g_crypto_buffer, 512 * (sector_end_aligned - device_sector_offset), 512, device->iv, device->crypt_flags);
        }
        no = sdmmc_write(device->mmc, g_crypto_buffer, sector_end_aligned, 1);
        if (no != 0) {
            r->_errno = no;
            return -1;
        }

        /* Advance */
        data += 512 - ((f->offset + len) % 512);
        current_sector++;
    }

    f->offset += len;
    return len;
}

static ssize_t rawmmcdev_read(struct _reent *r, void *fd, char *ptr, size_t len) {
    rawmmcdev_file_t *f = (rawmmcdev_file_t *)fd;
    rawmmcdev_device_t *device = f->device;
    uint32_t device_sector_offset = (uint32_t)(device->offset / 512);
    uint32_t sector_begin = (uint32_t)((device->offset + f->offset) / 512); /* NAND offset */
    uint32_t sector_end = (uint32_t)((device->offset + f->offset + len + 511) / 512);
    uint32_t sector_end_aligned = sector_end - ((f->offset + len) % 512 != 0 ? 1 : 0);
    uint32_t current_sector = sector_begin;
    uint8_t *data = (uint8_t *)ptr;

    int no = 0;

    if (f->offset + len >= device->offset + device->size) {
        len = device->size - f->offset;
    }

    /* Change the partition, if needed. */
    no = sdmmc_select_partition(device->mmc, device->partition);
    if (no != 0 && no != ENOTTY) {
        r->_errno = no;
        return -1;
    }

    /* Unaligned at the start. */
    if (f->offset % 512 != 0) {
        no = sdmmc_read(device->mmc, g_crypto_buffer, sector_begin, 1);
        if (no != 0) {
            r->_errno = no;
            return -1;
        }
        if (device->encrypted) {
            device->read_crypt_func(g_crypto_buffer, g_crypto_buffer, 512 * (sector_begin - device_sector_offset), 512, device->iv, device->crypt_flags);
        }
        memcpy(data, g_crypto_buffer + (f->offset % 512), len <= (512 - (f->offset % 512)) ? len : 512 - (f->offset % 512));

        /* Advance */
        data += 512 - (f->offset % 512);
        current_sector++;
    }

    /* Check if we're already done (otherwise this causes a bug in handling the last sector of the range). */
    if (current_sector == sector_end) {
        return 0;
    }

    size_t sectors_remaining = sector_end_aligned - current_sector;
    for (size_t i = 0; i < len; i += sizeof(g_crypto_buffer)/512) {
        size_t n = sectors_remaining <= sizeof(g_crypto_buffer)/512 ? sectors_remaining : sizeof(g_crypto_buffer)/512;

        if (device->encrypted) {
            no = sdmmc_read(device->mmc, g_crypto_buffer, current_sector, n);
            if (no != 0) {
                r->_errno = no;
                return -1;
            }
            device->read_crypt_func(g_crypto_buffer, g_crypto_buffer, current_sector - device_sector_offset, 512 * n, device->iv, device->crypt_flags);
            memcpy(data, g_crypto_buffer, 512 * n);
        } else {
            no = sdmmc_read(device->mmc, data, current_sector, n);
            if (no != 0) {
                r->_errno = no;
                return -1;
            }
        }

        data += 512 * n;
        current_sector += n;
    }

    /* Unaligned at the end, we need to read the sector and incorporate the data. */
    if (sector_end != sector_end_aligned) {
        no = sdmmc_read(device->mmc, g_crypto_buffer, sector_end_aligned, 1);
        if (no != 0) {
            r->_errno = no;
            return -1;
        }
        if (device->encrypted) {
            device->read_crypt_func(g_crypto_buffer, g_crypto_buffer, 512 * (sector_end_aligned - device_sector_offset), 512, device->iv, device->crypt_flags);
        }
        memcpy(data, g_crypto_buffer, (f->offset + len) % 512);

        /* Advance */
        data += 512 - (f->offset % 512);
        current_sector++;
    }

    f->offset += len;
    return len;
}

static off_t rawmmcdev_seek(struct _reent *r, void *fd, off_t pos, int whence) {
    rawmmcdev_file_t *f = (rawmmcdev_file_t *)fd;
    rawmmcdev_device_t *device = f->device;
    size_t off;

    switch (whence) {
        case SEEK_SET:
            off = 0;
            break;
        case SEEK_CUR:
            off = f->offset;
            break;
        case SEEK_END:
            off = device->size;
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

    return (off_t)(pos + off);
}

static void rawmmcdev_stat_impl(rawmmcdev_device_t *device, struct stat *st) {
    memset(st, 0, sizeof(struct stat));
    st->st_size = (off_t)(device->size);
    st->st_nlink = 1;

    st->st_blksize = 512;
    st->st_blocks = st->st_size / st->st_blksize;

    st->st_mode = S_IFBLK | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
}

static int rawmmcdev_fstat(struct _reent *r, void *fd, struct stat *st) {
    (void)r;
    rawmmcdev_file_t *f = (rawmmcdev_file_t *)fd;
    rawmmcdev_device_t *device = f->device;
    rawmmcdev_stat_impl(device, st);

    return 0;
}

static int rawmmcdev_stat(struct _reent *r, const char *file, struct stat *st) {
    rawmmcdev_device_t *device = (rawmmcdev_device_t *)(r->deviceData);
    if (strcmp(file, device->root_path) != 0) {
        r->_errno = ENOENT;
        return -1;
    }

    rawmmcdev_stat_impl(device, st);
    return 0;
}

static int rawmmcdev_fsync(struct _reent *r, void *fd) {
    /* Nothing to do. */
    (void)r;
    (void)fd;
    return 0;
}
