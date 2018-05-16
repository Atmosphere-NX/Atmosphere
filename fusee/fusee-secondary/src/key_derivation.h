#ifndef FUSEE_KEYDERIVATION_H
#define FUSEE_KEYDERIVATION_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* TODO: Update to 0x6 on release of new master key. */
#define MASTERKEY_REVISION_MAX 0x5

#define MASTERKEY_REVISION_100_230     0x00
#define MASTERKEY_REVISION_300         0x01
#define MASTERKEY_REVISION_301_302     0x02
#define MASTERKEY_REVISION_400_410     0x03
#define MASTERKEY_REVISION_500_CURRENT 0x04

#define MASTERKEY_NUM_NEW_DEVICE_KEYS (MASTERKEY_REVISION_MAX - MASTERKEY_REVISION_400_410)

typedef enum BisPartition {
    BisPartition_Calibration = 0,
    BisPartition_Safe = 1,
    BisPartition_UserSystem = 2,
} BisPartition;

typedef struct {
    uint8_t keys[9][0x10];
} nx_dec_keyblob_t;

typedef struct nx_keyblob_t {
    uint8_t mac[0x10];
    uint8_t ctr[0x10];
    union {
        uint8_t data[0x90];
        nx_dec_keyblob_t dec_blob;
    };
} nx_keyblob_t;

/* TSEC fw must be 0x100-byte-aligned. */
int derive_nx_keydata(uint32_t target_firmware, const nx_keyblob_t *keyblobs, uint32_t available_revision, const void *tsec_fw, size_t tsec_fw_size);
int load_package1_key(uint32_t revision);
void finalize_nx_keydata(uint32_t target_firmware);

void derive_bis_key(void *dst, BisPartition partition_id, uint32_t target_firmware);

#endif
