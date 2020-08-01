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

#ifndef FUSEE_DEVICE_PARTITION_H
#define FUSEE_DEVICE_PARTITION_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define DEVPART_IV_MAX_SIZE   16
#define DEVPART_KEY_MAX 2
#define DEVPART_KEY_MAX_SIZE 16

struct device_partition_t;

typedef int (*device_partition_initializer_t)(struct device_partition_t *devpart);
typedef void (*device_partition_finalizer_t)(struct device_partition_t *devpart);

/* Note: only random-access ciphers supporting in-place encryption/decryption are supported */
typedef int (*device_partition_cipher_t)(struct device_partition_t *devpart, uint64_t sector, uint64_t num_sectors);

typedef int (*device_partition_reader_t)(struct device_partition_t *devpart, void *dst, uint64_t sector, uint64_t num_sectors);
typedef int (*device_partition_writer_t)(struct device_partition_t *devpart, const void *src, uint64_t sector, uint64_t num_sectors);

typedef enum DevicePartitionCryptoMode {
    DevicePartitionCryptoMode_None,
    DevicePartitionCryptoMode_Ctr,
    DevicePartitionCryptoMode_Xts,
} DevicePartitionCryptoMode;

typedef struct device_partition_t {
    size_t sector_size; /* The size of a sector */
    uint64_t start_sector;    /* Offset in the parent device, in sectors. */
    uint64_t num_sectors;      /* Maximum size of the partition, in sectors (optional). */

    device_partition_cipher_t read_cipher; /* Cipher for read operations. */
    device_partition_cipher_t write_cipher; /* Cipher for write operations. */
    DevicePartitionCryptoMode crypto_mode; /* Mode to use for cryptographic operations. */
    size_t crypto_sector_size;

    device_partition_initializer_t initializer; /* Initializer. */
    device_partition_finalizer_t finalizer; /* Finalizer. */

    device_partition_reader_t reader; /* Reader. */
    device_partition_writer_t writer; /* Writer. */

    void *device_struct; /* Pointer to struct for additional info. */

    void *crypto_work_buffer; /* Work buffer for crypto. */
    uint64_t crypto_work_buffer_num_sectors; /* Size of the crypto work buffer in sectors. */

    uint8_t __attribute__((aligned(16))) keys[DEVPART_KEY_MAX][DEVPART_KEY_MAX_SIZE]; /* Key. */
    uint8_t __attribute__((aligned(16))) iv[DEVPART_IV_MAX_SIZE]; /* IV. */
    bool initialized;

    /* Emulation only. */
    bool is_emulated;
    bool emu_use_file;
    char emu_root_path[0x100 + 1];
    char emu_file_path[0x300 + 1];
    int emu_num_parts;
    uint64_t emu_part_limit;
} device_partition_t;

int device_partition_read_data(device_partition_t *devpart, void *dst, uint64_t sector, uint64_t num_sectors);
int device_partition_write_data(device_partition_t *devpart, const void *src, uint64_t sector, uint64_t num_sectors);

#endif
