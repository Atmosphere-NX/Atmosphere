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

#include <errno.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "../../../fusee/common/log.h"
#include "package1.h"
#include "bct.h"
#include "se.h"

static const uint8_t custom_public_key[0x100] = {
    0x59, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
    0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
    0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
    0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
    0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
    0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
    0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
    0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
    0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
    0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
    0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
    0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
    0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
    0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
    0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69,
    0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0x69, 0xFF,
};

static bool is_custom_public_key_erista(const void *bct) {
    return memcmp((const uint8_t *)bct + 0x210, custom_public_key, sizeof(custom_public_key)) == 0;
}

static bool is_custom_public_key_mariko(const void *bct) {
    return memcmp((const uint8_t *)bct + 0x10, custom_public_key, sizeof(custom_public_key)) == 0;
}

bool package1_is_custom_public_key(const void *bct, bool mariko) {
    if (mariko) {
        return is_custom_public_key_mariko(bct);
    } else {
        return is_custom_public_key_erista(bct);
    }
}

int package1_read_and_parse_boot0_erista(void **package1loader, size_t *package1loader_size, nx_keyblob_t *keyblobs, uint32_t *revision, FILE *boot0) {
    nvboot_config_table *bct; /* Normal firmware BCT, primary. TODO: check? */
    nv_bootloader_info *pk1l_info; /* TODO: check? */
    size_t fpos, pk1l_offset;
    union {
        nx_keyblob_t keyblob;
        uint8_t sector[0x200];
    } d;

    if (package1loader == NULL || package1loader_size == NULL || keyblobs == NULL || revision == NULL || boot0 == NULL) {
        errno = EINVAL;
        return -1;
    }

    bct = malloc(sizeof(nvboot_config_table));
    if (bct == NULL) {
        errno = ENOMEM;
        return -1;
    }
    pk1l_info = &bct->bootloader[0];

    fpos = ftell(boot0);

    /* Read the BCT. */
    if (fread(bct, sizeof(nvboot_config_table), 1, boot0) == 0) {
        free(bct);
        return -1;
    }

    /* If custom public key, use bct-2 instead of bct 0. */
    if (is_custom_public_key_erista(bct)) {
        if (fseek(boot0, fpos + 0x4000 * 2, SEEK_SET) != 0) {
            free(bct);
            return -1;
        }
        if (fread(bct, sizeof(nvboot_config_table), 1, boot0) == 0) {
            free(bct);
            return -1;
        }
    }

    if (bct->bootloader_used < 1 || pk1l_info->version < 1) {
        free(bct);
        errno = EILSEQ;
        return -1;
    }

    *revision = pk1l_info->version - 1;
    *package1loader_size = pk1l_info->length;

    pk1l_offset = 0x4000 * pk1l_info->start_blk + 0x200 * pk1l_info->start_page;
    free(bct);
    (*package1loader) = memalign(0x10000, *package1loader_size);

    if (*package1loader == NULL) {
        errno = ENOMEM;
        return -1;
    }

    /* Read the pk1/pk1l, and skip the backup too. */
    if (fseek(boot0, fpos + pk1l_offset, SEEK_SET) != 0) {
        return -1;
    }
    if (fread(*package1loader, *package1loader_size, 1, boot0) == 0) {
        return -1;
    }
    if (fseek(boot0, fpos + pk1l_offset + 2 * PACKAGE1LOADER_SIZE_MAX, SEEK_SET) != 0) {
        return -1;
    }

    /* Read the full keyblob area.*/
    for (size_t i = 0; i < 32; i++) {
        if (!fread(d.sector, 0x200, 1, boot0)) {
            return -1;
        }
        keyblobs[i] = d.keyblob;
    }

    return 0;
}

int package1_read_and_parse_boot0_mariko(void **package1loader, size_t *package1loader_size, FILE *boot0) {
    /* TODO: Actually parse BOOT0 to locate package1ldr. */
    size_t fpos, pk1l_offset;

    fpos = ftell(boot0);
    pk1l_offset = 0x100000;

    /* Read the OEM header. */
    pk11_mariko_oem_header_t oem_header;
    if (fseek(boot0, fpos + pk1l_offset, SEEK_SET) != 0) {
        return -1;
    }
    if (fread(&oem_header, sizeof(oem_header), 1, boot0) == 0) {
        return -1;
    }

    /* Sanity check the size. */
    if (oem_header.bl_size < 2 * sizeof(package1loader_header_t)) {
        return -1;
    }

    /* Set the size. */
    *package1loader_size = oem_header.bl_size;

    /* Allocate pk11. */
    (*package1loader) = memalign(0x10000, *package1loader_size);

    if (*package1loader == NULL) {
        errno = ENOMEM;
        return -1;
    }

    /* Read pk11. */
    if (fread(*package1loader, *package1loader_size, 1, boot0) == 0) {
        return -1;
    }

    /* Copy the plaintext header. */
    package1loader_header_t dec_header;
    memcpy(&dec_header, *package1loader, sizeof(dec_header));

    /* Decrypt the binary. */
    const uint8_t __attribute__((aligned(16))) iv[16] = {0};
    se_aes_128_cbc_decrypt(KEYSLOT_SWITCH_BEK_MARIKO, *package1loader, *package1loader_size, *package1loader, *package1loader_size, iv);

    /* Validate the decryption. */
    if (memcmp(&dec_header, (const uint8_t *)*package1loader + sizeof(dec_header), sizeof(dec_header)) != 0) {
        print(SCREEN_LOG_LEVEL_DEBUG, "Decrypted package1loader is invalid...\n");
        return -1;
    }

    /* Copy out the first header. */
    memcpy(*package1loader, &dec_header, sizeof(dec_header));

    return 0;
}

bool package1_get_tsec_fw(void **tsec_fw, const void *package1loader, size_t package1loader_size) {
    /* The TSEC firmware is always located at a 256-byte aligned address. */
    /* We're looking for its 4 first bytes. */
    const uint32_t *pos;
    uintptr_t pk1l = (uintptr_t)package1loader;
    for (pos = (const uint32_t *)pk1l; (uintptr_t)pos < pk1l + package1loader_size; pos += 0x40) {
        if (*pos == 0xCF42004D) {
            (*tsec_fw) = (void *)pos;
            return true;
        }
    }

    return false;
}

size_t package1_get_encrypted_package1(package1_header_t **package1, uint8_t *ctr, const void *package1loader, size_t package1loader_size) {
    const uint8_t *crypt_hdr = (const uint8_t *)package1loader + 0x4000 - 0x20;
    if (package1loader_size < 0x4000) {
        return 0; /* Shouldn't happen, ever. */
    }

    memcpy(ctr, crypt_hdr + 0x10, 0x10);
    (*package1) = (package1_header_t *)(crypt_hdr + 0x20);
    return *(uint32_t *)crypt_hdr;
}

bool package1_decrypt(package1_header_t *package1, size_t package1_size, const uint8_t *ctr) {
    uint8_t __attribute__((aligned(16))) ctrbuf[16];
    memcpy(ctrbuf, ctr, 16);
    se_aes_ctr_crypt(0xB, package1, package1_size, package1, package1_size, ctrbuf, 16);
    return memcmp(package1->magic, "PK11", 4) == 0;
}

void *package1_get_warmboot_fw(const package1_header_t *package1) {
    /*
        The layout of pk1 changes between versions.

        However, the secmon always starts by this erratum code:
        https://github.com/ARM-software/arm-trusted-firmware/blob/master/plat/nvidia/tegra/common/aarch64/tegra_helpers.S#L312
        and thus by 0xD5034FDF.

        Nx-bootloader starts by 0xE328F0C0 (msr cpsr_f, 0xc0) before 6.2.0 and by 0xF0C0A7F0 afterwards.
    */
    const uint32_t *data = (const uint32_t *)package1->data;
    for (size_t i = 0; i < 3; i++) {
        switch (*data) {
            case 0xD5034FDFu:
                data += package1->secmon_size / 4;
                break;
            case 0xE328F0C0:
            case 0xF0C0A7F0:
                data += package1->nx_bootloader_size / 4;
                break;
            default:
                /* TODO: should we validate its signature? */
                return (void *)data;
        }
    }

    return NULL;
}
