#ifndef FUSEE_MASTERKEY_H
#define FUSEE_MASTERKEY_H

/* This is glue code to enable master key support across versions. */

/* TODO: Update to 0x6 on release of new master key. */
#define MASTERKEY_REVISION_MAX 0x5

#define MASTERKEY_REVISION_100_230     0x00
#define MASTERKEY_REVISION_300         0x01
#define MASTERKEY_REVISION_301_302     0x02
#define MASTERKEY_REVISION_400_410     0x03
#define MASTERKEY_REVISION_500_CURRENT 0x04

/* This should be called during initialization. */
void mkey_detect_revision(void);

unsigned int mkey_get_revision(void);

unsigned int mkey_get_keyslot(unsigned int revision);

#endif