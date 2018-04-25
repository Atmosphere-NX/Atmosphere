#ifndef FUSEE_KEYDERIVATION_H
#define FUSEE_KEYDERIVATION_H

#include "hwinit/tsec.h"

/* TODO: Update to 0x6 on release of new master key. */
#define MASTERKEY_REVISION_MAX 0x5

#define MASTERKEY_REVISION_100_230     0x00
#define MASTERKEY_REVISION_300         0x01
#define MASTERKEY_REVISION_301_302     0x02
#define MASTERKEY_REVISION_400_410     0x03
#define MASTERKEY_REVISION_500_CURRENT 0x04

#define MASTERKEY_NUM_NEW_DEVICE_KEYS (MASTERKEY_REVISION_MAX - MASTERKEY_REVISION_400_410)

typedef enum {
    BisPartition_Calibration = 0,
    BisPartition_Safe = 1,
    BisPartition_UserSystem = 2
} BisPartition_t;

typedef struct {
    u8 mac[0x10];
    u8 ctr[0x10];
    union {
        u8 data[0x90];
        u8 keys[9][0x10];
    };
} nx_keyblob_t;

void derive_nx_keydata(u32 target_firmware);
void finalize_nx_keydata(u32 target_firmware);

void derive_bis_key(void *dst, BisPartition_t partition_id, u32 target_firmware);

#endif