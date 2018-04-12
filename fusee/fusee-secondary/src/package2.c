#include "utils.h"
#include "masterkey.h"
#include "stratosphere.h"
#include "package2.h"
#include "kip.h"
#include "se.h"
#include "lib/printk.h"

/* Stage 2 executes from DRAM, so we have tons of space. */
/* This *greatly* simplifies logic. */
unsigned char g_patched_package2[PACKAGE2_SIZE_MAX];
unsigned char g_package2_sections[PACKAGE2_SECTION_MAX][PACKAGE2_SIZE_MAX];

package2_header_t *g_patched_package2_header = (package2_header_t *)g_patched_package2; 

void package2_decrypt(void *package2_address);
void package2_add_thermosphere_section(void);
void package2_patch_kernel(void);
void package2_patch_ini1(void);
void package2_fixup_header_and_section_hashes(void);

void package2_patch(void *package2_address) {
    /* First things first: Decrypt Package2. */
    package2_decrypt(package2_address);
    
    /* Modify Package2 to add an additional thermosphere section. */
    package2_add_thermosphere_section();
    
    /* Perform any patches we want to the NX kernel. */
    package2_patch_kernel();
    
    /* Perform any patches we want to the INI1 (This is where our built-in sysmodules will be added.) */
    package2_patch_ini1();
    
    /* Fix all necessary data in the header to accomodate for the new patches. */
    package2_fixup_header_and_section_hashes();
    
    /* Relocate Package2. */
    memcpy(NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS, g_patched_package2, sizeof(g_patched_package2));
}

static void package2_crypt_ctr(unsigned int master_key_rev, void *dst, size_t dst_size, const void *src, size_t src_size, const void *ctr, size_t ctr_size) {
    /* Derive package2 key. */
    const uint8_t package2_key_source[0x10] = {0xFB, 0x8B, 0x6A, 0x9C, 0x79, 0x00, 0xC8, 0x49, 0xEF, 0xD2, 0x4D, 0x85, 0x4D, 0x30, 0xA0, 0xC7};
    unsigned int keyslot = mkey_get_keyslot(master_key_rev);
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_PACKAGE2KEY, keyslot, package2_key_source, 0x10);

    /* Perform Encryption. */
    se_aes_ctr_crypt(KEYSLOT_SWITCH_PACKAGE2KEY, dst, dst_size, src, src_size, ctr, ctr_size);
}

bool validate_package2_metadata(package2_meta_t *metadata) {
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

uint32_t decrypt_and_validate_header(package2_header_t *header, bool is_plaintext) {
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
            if (validate_package2_metadata(&metadata)) {
                header->metadata = metadata;
                return mkey_rev;
            }
        }

        /* Ensure we successfully decrypted the header. */
        if (mkey_rev > mkey_get_revision()) {   
            panic(0xFAF00003);
        }
    } else if (!validate_package2_metadata(&header->metadata)) {
        panic(0xFAF0003);
    }
    return 0;
}

void package2_decrypt(void *package2_address) {
    /* TODO: Actually decrypt, and copy sections into the relevant g_package2_sections[n] */
    memcpy(g_patched_package2, NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS, PACKAGE2_SIZE_MAX);
    
    bool is_package2_plaintext = g_patched_package2_header->signature[0];
    is_package2_plaintext &= memcmp(g_patched_package2_header->signature, g_patched_package2_header->signature + 1, sizeof(g_patched_package2_header->signature) - 1) == 0;
    is_package2_plaintext &= g_patched_package2_header->metadata.magic == MAGIC_PK21;
    
    uint32_t pk21_mkey_revision = decrypt_and_validate_header(g_patched_package2_header, is_package2_plaintext);
    
    size_t cur_section_offset = 0;
    /* Copy each section to its appropriate location, decrypting if necessary. */
    for (unsigned int section = 0; section < PACKAGE2_SECTION_MAX; section++) {
        if (g_patched_package2_header->metadata.section_sizes[section] == 0) {
            continue;
        }

        void *src_start = g_patched_package2 + sizeof(package2_header_t) + cur_section_offset;
        void *dst_start = g_package2_sections[section];
        size_t size = (size_t)g_patched_package2_header->metadata.section_sizes[section];

        if (is_package2_plaintext&& size != 0) {
            memcpy(dst_start, src_start, size);
        } else if (size != 0) {
            package2_crypt_ctr(pk21_mkey_revision, dst_start, size, src_start, size, g_patched_package2_header->metadata.section_ctrs[section], 0x10);
        }
        cur_section_offset += size;
    }
    
    /* Clear the signature, to signal that this is a plaintext, unsigned package2. */
    memset(g_patched_package2_header->signature, 0, sizeof(g_patched_package2_header->signature));
}

void package2_add_thermosphere_section(void) {
    if (g_patched_package2_header->metadata.section_sizes[PACKAGE2_SECTION_UNUSED] != 0) {
        printk("Error: Package2 has no unused section for Thermosph\xe8re!\n");
        generic_panic();
    }
    
    /* TODO: Copy thermosphere to g_package2_sections[PACKAGE2_SECTION_UNUSED], update header size. */
}

void package2_patch_kernel(void) {
    /* TODO: What kind of patching do we want to try to do here? */
}


void package2_patch_ini1(void) {
    /* TODO: Do we want to support loading another INI from sd:/whatever/INI1.bin? */
    ini1_header_t *inis_to_merge[STRATOSPHERE_INI1_MAX] = {0};
    inis_to_merge[STRATOSPHERE_INI1_EMBEDDED] = stratosphere_get_ini1();
    inis_to_merge[STRATOSPHERE_INI1_PACKAGE2] = (ini1_header_t *)g_package2_sections[PACKAGE2_SECTION_INI1];

    /* Merge all of the INI1s. */
    g_patched_package2_header->metadata.section_sizes[PACKAGE2_SECTION_INI1] = stratosphere_merge_inis(g_package2_sections[PACKAGE2_SECTION_INI1], inis_to_merge, STRATOSPHERE_INI1_MAX);   
}

void package2_fixup_header_and_section_hashes(void) {
    size_t cur_section_offset = 0;
    
    /* Copy each section to its appropriate location. */
    for (unsigned int section = 0; section < PACKAGE2_SECTION_MAX; section++) {
        if (g_patched_package2_header->metadata.section_sizes[section] == 0) {
            continue;
        }
        
        size_t size = (size_t)g_patched_package2_header->metadata.section_sizes[section];
        if (sizeof(package2_header_t) + cur_section_offset + size > PACKAGE2_SIZE_MAX) {
            printk("Error: Patched Package2 is too big!\n");
            generic_panic();
        }
        
        /* Copy the section into the new package2. */
        memcpy(g_patched_package2 + sizeof(package2_header_t) + cur_section_offset, g_package2_sections[section], size);
        /* Fix up the hash. */
        se_calculate_sha256(g_patched_package2_header->metadata.section_hashes[section], g_package2_sections[section], size);
        
        cur_section_offset += size;
    }
    
    /* Fix up the size in XOR'd CTR. */
    uint32_t package_size = g_patched_package2_header->metadata.ctr_dwords[0] ^ g_patched_package2_header->metadata.ctr_dwords[2] ^ g_patched_package2_header->metadata.ctr_dwords[3];
    uint32_t new_package_size = sizeof(package2_header_t) + cur_section_offset;
    g_patched_package2_header->metadata.ctr_dwords[3] ^= (package_size ^ new_package_size);
}