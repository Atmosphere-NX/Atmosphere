#ifndef EXOSPHERE_MASTERKEY_H
#define EXOSPHERE_MASTERKEY_H

/* This is glue code to enable master key support across versions. */

/* TODO: Update to 0x6 on release of new master key. */
#define MASTERKEY_REVISION_MAX 0x5

#define MASTERKEY_REVISION_100_230     0x00
#define MASTERKEY_REVISION_300         0x01
#define MASTERKEY_REVISION_301_302     0x02
#define MASTERKEY_REVISION_400_410     0x03
#define MASTERKEY_REVISION_500_CURRENT 0x04

#define MASTERKEY_NUM_NEW_DEVICE_KEYS (MASTERKEY_REVISION_MAX - MASTERKEY_REVISION_400_410)

/* This should be called early on in initialization. */
void mkey_detect_revision(void);

unsigned int mkey_get_revision(void);

unsigned int mkey_get_keyslot(unsigned int revision);

void set_old_devkey(unsigned int revision, const uint8_t *key);
unsigned int devkey_get_keyslot(unsigned int revision);

#endif