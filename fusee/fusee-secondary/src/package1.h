#ifndef FUSEE_PACKAGE1_H
#define FUSEE_PACKAGE1_H

#include <stdio.h>
#include "key_derivation.h"

#define PACKAGE1LOADER_SIZE_MAX 0x40000

typedef struct package1_header_t {
    char magic[4];
    uint32_t warmboot_size;
    uint32_t _0x8;
    uint32_t _0xC;
    uint32_t nx_bootloader_size;
    uint32_t _0x14;
    uint32_t secmon_size;
    uint32_t _0x1C;
    uint8_t data[];
} package1_header_t;

int package1_read_and_parse_boot0(void **package1loader, size_t *package1loader_size, nx_keyblob_t *keyblobs, uint32_t *revision, FILE *boot0);

size_t package1_get_tsec_fw(void **tsec_fw, const void *package1loader, size_t package1loader_size);
size_t package1_get_encrypted_package1(package1_header_t **package1, uint8_t *ctr, const void *package1loader, size_t package1loader_size);

/* Must be aligned to 16 bytes. */
bool package1_decrypt(package1_header_t *package1, size_t package1_size, const uint8_t *ctr);
void *package1_get_warmboot_fw(const package1_header_t *package1);

#endif
