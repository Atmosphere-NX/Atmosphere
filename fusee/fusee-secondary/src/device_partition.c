#include <string.h>
#include "device_partition.h"

int device_partition_read_data(device_partition_t *devpart, void *dst, uint64_t sector, uint64_t num_sectors)
{
    int rc;
    if (!devpart->initialized) {
        rc = devpart->initializer(devpart);
        if (rc != 0) {
            return rc;
        }
    }
    if (devpart->read_cipher != NULL && devpart->crypto_mode != DevicePartitionCryptoMode_None) {
        for (uint64_t i = 0; i < num_sectors; i += devpart->crypto_work_buffer_num_sectors) {
            uint64_t n = (i + devpart->crypto_work_buffer_num_sectors > num_sectors) ? (num_sectors - i) : devpart->crypto_work_buffer_num_sectors;
            rc = devpart->reader(devpart, devpart->crypto_work_buffer, sector + i, n);
            if (rc != 0) {
                return rc;
            }
            rc = devpart->read_cipher(devpart, sector + i, n);
            if (rc != 0) {
                return rc;
            }
            memcpy(dst + (size_t)(devpart->sector_size * i), devpart->crypto_work_buffer, (size_t)(devpart->sector_size * n)); 
        }
        return 0;
    } else {
        return devpart->reader(devpart, dst, sector, num_sectors);
    }
}

int device_partition_write_data(device_partition_t *devpart, const void *src, uint64_t sector, uint64_t num_sectors)
{
    int rc;
    if (!devpart->initialized) {
        rc = devpart->initializer(devpart);
        if (rc != 0) {
            return rc;
        }
    }
    if (devpart->read_cipher != NULL && devpart->crypto_mode != DevicePartitionCryptoMode_None) {
        for (uint64_t i = 0; i < num_sectors; i += devpart->crypto_work_buffer_num_sectors) {
            uint64_t n = (i + devpart->crypto_work_buffer_num_sectors > num_sectors) ? (num_sectors - i) : devpart->crypto_work_buffer_num_sectors;
            memcpy(devpart->crypto_work_buffer, src + (size_t)(devpart->sector_size * i), (size_t)(devpart->sector_size * n)); 
            rc = devpart->write_cipher(devpart, sector + i, n);
            if (rc != 0) {
                return rc;
            }
            rc = devpart->writer(devpart, devpart->crypto_work_buffer, sector + i, n);
            if (rc != 0) {
                return rc;
            }
        }
        return 0;
    } else {
        return devpart->writer(devpart, src, sector, num_sectors);
    }
}
