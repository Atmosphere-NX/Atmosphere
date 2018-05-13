#ifndef FUSEE_DEVICE_PARTITION_H
#define FUSEE_DEVICE_PARTITION_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define DEVPART_IV_MAX_SIZE   16

struct device_partition_t;

typedef int (*device_partition_initializer_t)(struct device_partition_t *devpart);
typedef void (*device_partition_finalizer_t)(struct device_partition_t *devpart);

/* Note: only random-access ciphers supporting in-place encryption/decryption are supported */
typedef int (*device_partition_cipher_t)(struct device_partition_t *devpart, uint64_t sector, uint64_t num_sectors);

typedef int (*device_partition_reader_t)(struct device_partition_t *devpart, void *dst, uint64_t sector, uint64_t num_sectors);
typedef int (*device_partition_writer_t)(struct device_partition_t *devpart, const void *src, uint64_t sector, uint64_t num_sectors);

typedef struct device_partition_t {
    size_t sector_size; /* The size of a sector */
    uint64_t start_sector;    /* Offset in the parent device, in sectors. */
    uint64_t num_sectors;      /* Maximum size of the partition, in sectors (optional). */

    device_partition_cipher_t read_cipher; /* Cipher for read operations. */
    device_partition_cipher_t write_cipher; /* Cipher for write operations. */
    uint64_t crypto_flags; /* Additional information for crypto, for conveniency. */

    device_partition_initializer_t initializer; /* Initializer. */
    device_partition_finalizer_t finalizer; /* Finalizer. */

    device_partition_reader_t reader; /* Reader. */
    device_partition_writer_t writer; /* Writer. */

    void *device_struct; /* Pointer to struct for additional info. */

    void *crypto_work_buffer; /* Work buffer for crypto. */
    uint64_t crypto_work_buffer_num_sectors; /* Size of the crypto work buffer in sectors. */

    uint8_t iv[DEVPART_IV_MAX_SIZE]; /* IV. */
    bool initialized;
} device_partition_t;

int device_partition_read_data(device_partition_t *devpart, void *dst, uint64_t sector, uint64_t num_sectors);
int device_partition_write_data(device_partition_t *devpart, const void *src, uint64_t sector, uint64_t num_sectors);

#endif
