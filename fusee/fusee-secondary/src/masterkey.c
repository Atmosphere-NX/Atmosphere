#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "masterkey.h"
#include "se.h"

static unsigned int g_mkey_revision = 0;
static bool g_determined_mkey_revision = false;

static uint8_t g_old_masterkeys[MASTERKEY_REVISION_MAX][0x10];

/* TODO: Extend with new vectors, as needed. */
static const uint8_t mkey_vectors[MASTERKEY_REVISION_MAX][0x10] =
{
    {0x0C, 0xF0, 0x59, 0xAC, 0x85, 0xF6, 0x26, 0x65, 0xE1, 0xE9, 0x19, 0x55, 0xE6, 0xF2, 0x67, 0x3D}, /* Zeroes encrypted with Master Key 00. */
    {0x29, 0x4C, 0x04, 0xC8, 0xEB, 0x10, 0xED, 0x9D, 0x51, 0x64, 0x97, 0xFB, 0xF3, 0x4D, 0x50, 0xDD}, /* Master key 00 encrypted with Master key 01. */
    {0xDE, 0xCF, 0xEB, 0xEB, 0x10, 0xAE, 0x74, 0xD8, 0xAD, 0x7C, 0xF4, 0x9E, 0x62, 0xE0, 0xE8, 0x72}, /* Master key 01 encrypted with Master key 02. */
    {0x0A, 0x0D, 0xDF, 0x34, 0x22, 0x06, 0x6C, 0xA4, 0xE6, 0xB1, 0xEC, 0x71, 0x85, 0xCA, 0x4E, 0x07}, /* Master key 02 encrypted with Master key 03. */
    {0x6E, 0x7D, 0x2D, 0xC3, 0x0F, 0x59, 0xC8, 0xFA, 0x87, 0xA8, 0x2E, 0xD5, 0x89, 0x5E, 0xF3, 0xE9}, /* Master key 03 encrypted with Master key 04. */
};

bool check_mkey_revision(unsigned int revision) {
    uint8_t final_vector[0x10];

    unsigned int check_keyslot = KEYSLOT_SWITCH_MASTERKEY;
    if (revision > 0) {
        /* Generate old master key array. */
        for (unsigned int i = revision; i > 0; i--) {
            se_aes_ecb_decrypt_block(check_keyslot, g_old_masterkeys[i-1], 0x10, mkey_vectors[i], 0x10);
            set_aes_keyslot(KEYSLOT_SWITCH_TEMPKEY, g_old_masterkeys[i-1], 0x10);
            check_keyslot = KEYSLOT_SWITCH_TEMPKEY;
        }
    }

    se_aes_ecb_decrypt_block(check_keyslot, final_vector, 0x10, mkey_vectors[0], 0x10);
    for (unsigned int i = 0; i < 0x10; i++) {
        if (final_vector[i] != 0) {
            return false;
        }
    }
    return true;
}

void mkey_detect_revision(void) {
    if (g_determined_mkey_revision) {
        generic_panic();
    }
    
    for (unsigned int rev = 0; rev < MASTERKEY_REVISION_MAX; rev++) {
        if (check_mkey_revision(rev)) {
            g_determined_mkey_revision = true;
            g_mkey_revision = rev;
            break;
        }
    }
    
    /* We must have determined the master key, or we're not running on a Switch. */
    if (!g_determined_mkey_revision) {
        /* Panic in bright red. */
        panic(0x00F00060);
    }
}

unsigned int mkey_get_revision(void) {
    if (!g_determined_mkey_revision) {
        generic_panic();
    }

    return g_mkey_revision;
}

unsigned int mkey_get_keyslot(unsigned int revision) {
    if (!g_determined_mkey_revision || revision >= MASTERKEY_REVISION_MAX) {
        generic_panic();
    }

    if (revision > g_mkey_revision) {
        generic_panic();
    }

    if (revision == g_mkey_revision) {
        return KEYSLOT_SWITCH_MASTERKEY;
    } else {
        /* Load into a temp keyslot. */
        set_aes_keyslot(KEYSLOT_SWITCH_TEMPKEY, g_old_masterkeys[revision], 0x10);
        return KEYSLOT_SWITCH_TEMPKEY;
    }
}