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

#include <stdio.h>
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
    if ((devpart->read_cipher != NULL) && (devpart->crypto_mode != DevicePartitionCryptoMode_None)) {
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
    if ((devpart->write_cipher != NULL) && (devpart->crypto_mode != DevicePartitionCryptoMode_None)) {
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

int emu_device_partition_read_data(device_partition_t *devpart, void *dst, uint64_t sector, uint64_t num_sectors, const char *origin_path, int num_parts, uint64_t part_limit)
{
    int rc = 0;
    uint64_t target_sector = sector;
    char target_path[0x300 + 1] = {0};
    
    /* Perform initialization steps, if necessary. */
    if (!devpart->initialized) {
        rc = devpart->initializer(devpart);
        if (rc != 0) {
            return rc;
        }
    }
    
    /* Prepare the right file path if using file mode. */
    if (devpart->emu_use_file && (origin_path != NULL)) {
        /* Handle data in multiple parts, if necessary. */
        if (num_parts > 0) {
            int target_part = 0;
            uint64_t data_offset = sector * devpart->sector_size;
            
            if (data_offset >= part_limit) {
                uint64_t data_offset_aligned = (data_offset + (part_limit - 1)) & ~(part_limit - 1);
                target_part = (data_offset_aligned == data_offset) ? (data_offset / part_limit) : (data_offset_aligned / part_limit) - 1;
                target_sector = (data_offset - (target_part * part_limit)) / devpart->sector_size;
                
                /* Target part is invalid. */
                if (target_part > num_parts) {
                    return -1;
                }
            }
            
            /* Treat the path as a folder with each part inside. */
            snprintf(target_path, sizeof(target_path) - 1, "%s/%02d", origin_path, target_part);
        } else {
            /* If there are no parts, copy the origin path directly. */
            strcpy(target_path, origin_path);
        }
        
        /* Update the target file path. */
        devpart->emu_file_path = target_path;
    }
    
    /* Read the partition data. */
    if ((devpart->read_cipher != NULL) && (devpart->crypto_mode != DevicePartitionCryptoMode_None)) {
        for (uint64_t i = 0; i < num_sectors; i += devpart->crypto_work_buffer_num_sectors) {
            uint64_t n = (i + devpart->crypto_work_buffer_num_sectors > num_sectors) ? (num_sectors - i) : devpart->crypto_work_buffer_num_sectors;
            
            /* Read partition data. */
            rc = devpart->reader(devpart, devpart->crypto_work_buffer, target_sector + i, n);
            
            if (rc != 0) {
                return rc;
            }
            
            /* Decrypt partition data. */
            rc = devpart->read_cipher(devpart, target_sector + i, n);
            
            if (rc != 0) {
                return rc;
            }
            
            /* Copy partition data to destination. */
            memcpy(dst + (size_t)(devpart->sector_size * i), devpart->crypto_work_buffer, (size_t)(devpart->sector_size * n)); 
        }
    } else {
        /* Read partition data. */
        rc = devpart->reader(devpart, dst, target_sector, num_sectors);
    }
    
    return rc;
}

int emu_device_partition_write_data(device_partition_t *devpart, const void *src, uint64_t sector, uint64_t num_sectors, const char *origin_path, int num_parts, uint64_t part_limit)
{
    int rc = 0;
    uint64_t target_sector = sector;
    char target_path[0x300 + 1] = {0};
    
    /* Perform initialization steps, if necessary. */
    if (!devpart->initialized) {
        rc = devpart->initializer(devpart);
        if (rc != 0) {
            return rc;
        }
    }
    
    /* Prepare the right file path if using file mode. */
    if (devpart->emu_use_file && (origin_path != NULL)) {
        /* Handle data in multiple parts, if necessary. */
        if (num_parts > 0) {
            int target_part = 0;
            uint64_t data_offset = sector * devpart->sector_size;
            
            if (data_offset >= part_limit) {
                uint64_t data_offset_aligned = (data_offset + (part_limit - 1)) & ~(part_limit - 1);
                target_part = (data_offset_aligned == data_offset) ? (data_offset / part_limit) : (data_offset_aligned / part_limit) - 1;
                target_sector = (data_offset - (target_part * part_limit)) / devpart->sector_size;
                
                /* Target part is invalid. */
                if (target_part > num_parts) {
                    return -1;
                }
            }
            
            /* Treat the path as a folder with each part inside. */
            snprintf(target_path, sizeof(target_path) - 1, "%s/%02d", origin_path, target_part);
        } else {
            /* If there are no parts, copy the origin path directly. */
            strcpy(target_path, origin_path);
        }
        
        /* Update the target file path. */
        devpart->emu_file_path = target_path;
    }
    
    /* Write the partition data. */
    if ((devpart->write_cipher != NULL) && (devpart->crypto_mode != DevicePartitionCryptoMode_None)) {
        for (uint64_t i = 0; i < num_sectors; i += devpart->crypto_work_buffer_num_sectors) {
            uint64_t n = (i + devpart->crypto_work_buffer_num_sectors > num_sectors) ? (num_sectors - i) : devpart->crypto_work_buffer_num_sectors;
            
            /* Copy partition data from source. */
            memcpy(devpart->crypto_work_buffer, src + (size_t)(devpart->sector_size * i), (size_t)(devpart->sector_size * n)); 
            
            /* Encrypt data. */
            rc = devpart->write_cipher(devpart, target_sector + i, n);
            
            if (rc != 0) {
                return rc;
            }
            
            /* Write partition data. */
            rc = devpart->writer(devpart, devpart->crypto_work_buffer, target_sector + i, n);
            
            if (rc != 0) {
                return rc;
            }
        }
    } else {
        /* Write partition data. */
        rc = devpart->writer(devpart, src, target_sector, num_sectors);
    }
    
    return rc;
}
