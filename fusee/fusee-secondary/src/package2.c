#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "masterkey.h"
#include "stratosphere.h"
#include "package2.h"
#include "kip.h"
#include "se.h"

static void package2_decrypt(package2_header_t *package2);
static size_t package2_get_thermosphere(void **thermosphere);
static void package2_patch_kernel(void *kernel, size_t kernel_size);
static ini1_header_t *package2_rebuild_ini1(ini1_header_t *ini1, uint32_t target_firmware);
static void package2_append_section(size_t id, package2_header_t *package2, void *data, size_t size);
static void package2_fixup_header_and_section_hashes(package2_header_t *package2, size_t size);

void package2_rebuild_and_copy(package2_header_t *package2, uint32_t target_firmware) {
    uintptr_t pk2da = (uintptr_t)package2 + sizeof(package2_header_t);
    package2_header_t *rebuilt_package2;
    size_t rebuilt_package2_size;
    void *kernel;
    size_t kernel_size;
    void *thermosphere;
    size_t thermosphere_size;
    ini1_header_t *rebuilt_ini1;

    /* First things first: Decrypt Package2 in place. */
    package2_decrypt(package2);
    kernel = (void *)(pk2da + package2->metadata.section_offsets[PACKAGE2_SECTION_KERNEL]);
    kernel_size = package2->metadata.section_sizes[PACKAGE2_SECTION_KERNEL];

    /* Modify Package2 to add an additional thermosphere section. */
    thermosphere_size = package2_get_thermosphere(&thermosphere);

    if (thermosphere_size != 0 && package2->metadata.section_sizes[PACKAGE2_SECTION_UNUSED] != 0) {
        printf(u8"Error: Package2 has no unused section for Thermosphère!\n");
        generic_panic();
    }

    /* Perform any patches we want to the NX kernel. */
    package2_patch_kernel(kernel, kernel_size);

    /* Perform any patches to the INI1, rebuilding it (This is where our built-in sysmodules will be added.) */
    rebuilt_ini1 = package2_rebuild_ini1((ini1_header_t *)(pk2da + package2->metadata.section_offsets[PACKAGE2_SECTION_INI1]), target_firmware);

    /* Allocate the rebuilt package2. */
    rebuilt_package2_size = sizeof(package2_header_t);
    rebuilt_package2_size += package2->metadata.section_sizes[PACKAGE2_SECTION_KERNEL] + thermosphere_size + rebuilt_ini1->size;

    if (rebuilt_package2_size > PACKAGE2_SIZE_MAX) {
        printf("Error: rebuilt package2 is too big!\n");
        generic_panic();
    }

    rebuilt_package2 = (package2_header_t *)malloc(rebuilt_package2_size);
    if (rebuilt_package2 == NULL) {
        printf("Error: package2_rebuild: out of memory!\n");
        generic_panic();
    }

    /* Rebuild package2. */
    memcpy(rebuilt_package2, package2, sizeof(package2_header_t));
    package2_append_section(PACKAGE2_SECTION_KERNEL, rebuilt_package2, kernel, kernel_size);
    package2_append_section(PACKAGE2_SECTION_INI1, rebuilt_package2, rebuilt_ini1, rebuilt_ini1->size);
    package2_append_section(PACKAGE2_SECTION_UNUSED, rebuilt_package2, thermosphere, thermosphere_size);

    /* Fix all necessary data in the header to accomodate for the new patches. */
    package2_fixup_header_and_section_hashes(rebuilt_package2, rebuilt_package2_size);

    /* Relocate Package2. */
    memcpy(NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS, rebuilt_package2, PACKAGE2_SIZE_MAX);

    /* We're done. */
    free(rebuilt_ini1);
    free(rebuilt_package2);
}


static void package2_crypt_ctr(unsigned int master_key_rev, void *dst, size_t dst_size, const void *src, size_t src_size, const void *ctr, size_t ctr_size) {
    /* Derive package2 key. */
    const uint8_t package2_key_source[0x10] = {0xFB, 0x8B, 0x6A, 0x9C, 0x79, 0x00, 0xC8, 0x49, 0xEF, 0xD2, 0x4D, 0x85, 0x4D, 0x30, 0xA0, 0xC7};
    unsigned int keyslot = mkey_get_keyslot(master_key_rev);
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_PACKAGE2KEY, keyslot, package2_key_source, 0x10);

    /* Perform Encryption. */
    se_aes_ctr_crypt(KEYSLOT_SWITCH_PACKAGE2KEY, dst, dst_size, src, src_size, ctr, ctr_size);
}

static bool package2_validate_metadata(package2_meta_t *metadata) {
    if (metadata->magic != MAGIC_PK21) {
        return false;
    }

    /* Package2 size, version number is stored XORed in header CTR. */
    /* Nintendo, what the fuck? */
    uint32_t package_size = metadata->ctr_dwords[0] ^ metadata->ctr_dwords[2] ^ metadata->ctr_dwords[3];
    uint8_t header_version = (uint8_t)((metadata->ctr_dwords[1] ^ (metadata->ctr_dwords[1] >> 16) ^ (metadata->ctr_dwords[1] >> 24)) & 0xFF);

    /* Ensure package isn't too big or too small. */
    if (package_size <= sizeof(package2_header_t) || package_size > PACKAGE2_SIZE_MAX - sizeof(package2_header_t)) {
        return false;
    }

    /* Validate that we're working with a header we know how to handle. */
    if (header_version > MASTERKEY_REVISION_MAX) {
        return false;
    }

    /* Require aligned entrypoint. */
    if (metadata->entrypoint & 3) {
        return false;
    }

    /* Validate section size sanity. */
    if (metadata->section_sizes[0] + metadata->section_sizes[1] + metadata->section_sizes[2] + sizeof(package2_header_t) != package_size) {
        return false;
    }

    bool entrypoint_found = false;

    /* Header has space for 4 sections, but only 3 are validated/potentially loaded on hardware. */
    size_t cur_section_offset = 0;
    for (unsigned int section = 0; section < PACKAGE2_SECTION_MAX; section++) {
        /* Validate section size alignment. */
        if (metadata->section_sizes[section] & 3) {
            return false;
        }

        /* Validate section does not overflow. */
        if (check_32bit_additive_overflow(metadata->section_offsets[section], metadata->section_sizes[section])) {
            return false;
        }

        /* Check for entrypoint presence. */
        uint32_t section_end = metadata->section_offsets[section] + metadata->section_sizes[section];
        if (metadata->section_offsets[section] <= metadata->entrypoint && metadata->entrypoint < section_end) {
            entrypoint_found = true;
        }

        /* Ensure no overlap with later sections. */
        for (unsigned int later_section = section + 1; later_section < PACKAGE2_SECTION_MAX; later_section++) {
            uint32_t later_section_end = metadata->section_offsets[later_section] + metadata->section_sizes[later_section];
            if (overlaps(metadata->section_offsets[section], section_end, metadata->section_offsets[later_section], later_section_end)) {
                return false;
            }
        }

        /* Validate section hashes. */
        if (metadata->section_sizes[section]) {
            void *section_data = (void *)((uint8_t *)NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS + sizeof(package2_header_t) + cur_section_offset);
            uint8_t calculated_hash[0x20];
            se_calculate_sha256(calculated_hash, section_data, metadata->section_sizes[section]);
            if (memcmp(calculated_hash, metadata->section_hashes[section], sizeof(metadata->section_hashes[section])) != 0) {
                return false;
            }
            cur_section_offset += metadata->section_sizes[section];
        }

    }

    /* Ensure that entrypoint is present in one of our sections. */
    if (!entrypoint_found) {
        return false;
    }

    /* Perform version checks. */
    /* We will be compatible with all package2s released before current, but not newer ones. */
    if (metadata->version_max >= PACKAGE2_MINVER_THEORETICAL && metadata->version_min < PACKAGE2_MAXVER_500_CURRENT) {
        return true;
    }

    return false;
}

static uint32_t package2_decrypt_and_validate_header(package2_header_t *header, bool is_plaintext) {
    package2_meta_t metadata;

    /* TODO: Also accept plaintext package2 based on bootconfig. */

    if (!is_plaintext) {
        uint32_t mkey_rev;

        /* Try to decrypt for all possible master keys. */
        for (mkey_rev = 0; mkey_rev <= mkey_get_revision(); mkey_rev++) {
            package2_crypt_ctr(mkey_rev, &metadata, sizeof(package2_meta_t), &header->metadata, sizeof(package2_meta_t), header->metadata.ctr, sizeof(header->metadata.ctr));
            /* Copy the ctr (which stores information) into the decrypted metadata. */
            memcpy(metadata.ctr, header->metadata.ctr, sizeof(header->metadata.ctr));
            /* See if this is the correct key. */
            if (package2_validate_metadata(&metadata)) {
                header->metadata = metadata;
                return mkey_rev;
            }
        }

        /* Ensure we successfully decrypted the header. */
        if (mkey_rev > mkey_get_revision()) {
            panic(0xFAF00003);
        }
    } else if (!package2_validate_metadata(&header->metadata)) {
        panic(0xFAF0003);
    }
    return 0;
}

static void package2_decrypt(package2_header_t *package2) {
    bool is_package2_plaintext = package2->signature[0];
    is_package2_plaintext &= memcmp(package2->signature, package2->signature + 1, sizeof(package2->signature) - 1) == 0;
    is_package2_plaintext &= package2->metadata.magic == MAGIC_PK21;

    uint32_t pk21_mkey_revision = package2_decrypt_and_validate_header(package2, is_package2_plaintext);

    size_t cur_section_offset = 0;
    /* Copy each section to its appropriate location, decrypting if necessary. */
    for (unsigned int section = 0; section < PACKAGE2_SECTION_MAX; section++) {
        if (package2->metadata.section_sizes[section] == 0) {
            continue;
        }

        void *src_start = (uint8_t *)package2 + sizeof(package2_header_t) + cur_section_offset;
        void *dst_start = src_start;
        size_t size = (size_t)package2->metadata.section_sizes[section];

        if (is_package2_plaintext&& size != 0) {
            memcpy(dst_start, src_start, size);
        } else if (size != 0) {
            package2_crypt_ctr(pk21_mkey_revision, dst_start, size, src_start, size, package2->metadata.section_ctrs[section], 0x10);
        }
        cur_section_offset += size;
    }

    /* Clear the signature, to signal that this is a plaintext, unsigned package2. */
    memset(package2->signature, 0, sizeof(package2->signature));
}

static size_t package2_get_thermosphere(void **thermosphere) {
    /*extern const uint8_t thermosphere_bin[];
    extern const uint32_t thermosphere_bin_size;*/
    /* TODO: enable when tested. */
    (*thermosphere) = NULL;
    return 0;
}

static void package2_patch_kernel(void *kernel, size_t size) {
    (void)kernel;
    (void)size;
    /* TODO: What kind of patching do we want to try to do here? */
}


static ini1_header_t *package2_rebuild_ini1(ini1_header_t *ini1, uint32_t target_firmware) {
    /* TODO: Do we want to support loading another INI from sd:/whatever/INI1.bin? */
    ini1_header_t *inis_to_merge[STRATOSPHERE_INI1_MAX] = {0};
    ini1_header_t *merged;

    inis_to_merge[STRATOSPHERE_INI1_EMBEDDED] = stratosphere_get_ini1(target_firmware);
    inis_to_merge[STRATOSPHERE_INI1_PACKAGE2] = ini1;

    /* Merge all of the INI1s. */
    merged = stratosphere_merge_inis(inis_to_merge, STRATOSPHERE_INI1_MAX);

    /* Free temporary buffer. */
    stratosphere_free_ini1();

    return merged;
}

static void package2_append_section(size_t id, package2_header_t *package2, void *data, size_t size) {
    /* This function must be called in ascending order of id */
    uint32_t offset = id == 0 ? 0 : package2->metadata.section_offsets[id - 1] + package2->metadata.section_sizes[id - 1];
    void *dst = (uint8_t *)package2 + sizeof(package2_header_t) + offset;
    memcpy(dst, data, size);

    package2->metadata.section_offsets[id] = offset;
    package2->metadata.section_sizes[id] = ((size + 3) << 2) >> 2;
}

static void package2_fixup_header_and_section_hashes(package2_header_t *package2, size_t size) {
    /* Copy each section to its appropriate location. */
    for (unsigned int section = 0; section < PACKAGE2_SECTION_MAX; section++) {
        if (package2->metadata.section_sizes[section] == 0) {
            continue;
        }

        size_t sz = (size_t)package2->metadata.section_sizes[section];
        size_t off = sizeof(package2_header_t) + package2->metadata.section_offsets[section];

        /* Fix up the hash. */
        se_calculate_sha256(package2->metadata.section_hashes[section],(uint8_t *)package2 + off, sz);
    }

    /* Fix up the size in XOR'd CTR. */
    uint32_t package_size = package2->metadata.ctr_dwords[0] ^ package2->metadata.ctr_dwords[2] ^ package2->metadata.ctr_dwords[3];
    package2->metadata.ctr_dwords[3] ^= (package_size ^ size);
}
