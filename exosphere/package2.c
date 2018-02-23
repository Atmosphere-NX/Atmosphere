#include <stdbool.h>
#include <stdint.h>

#include "utils.h"

#include "package2.h"
#include "configitem.h"
#include "se.h"
#include "masterkey.h"
#include "cache.h"
#include "randomcache.h"
#include "timers.h"

void setup_mmio_virtual_addresses(void) {
    /* TODO: Set Timers address to 0x1F008B000. */
    /* TODO: Set Security Engine address to 0x1F008F000. */
    /* TODO: Set CAR address to 0x1F0087000. */
    /* TODO: Set PMC address to 0x1F0089400. */
    /* TODO: Set Fuse address to 0x1F0096800. */
    /* TODO: Set Interrupt addresses to 0x1F0080000, 0x1F0082000. */
    /* TODO: Set Flow Controller address to 0x1F009D000. */
    /* TODO: Set UART-A address to 0x1F0085000. */
    /* TODO: Set I2C-0 address to 0x1F00A5000. */
    /* TODO: Set I2C-4 address to 0x1F00A1000. */
    /* TODO: Set MISC address to 0x1F0098000. */
    /* TODO: Set GPIO-1 address to 0x1F00A3000. */
}

/* Hardware init, sets up the RNG and SESSION keyslots, derives new DEVICE key. */
void setup_se(void) {
    uint8_t work_buffer[0x10];
    
    /* Sanity check the Security Engine. */
    se_verify_flags_cleared();
    se_clear_interrupts();
    
    /* Perform some sanity initialization. */
    security_engine_t *p_security_engine = get_security_engine_address();
    p_security_engine->_0x4 = 0;
    p_security_engine->AES_KEY_READ_DISABLE_REG = 0;
    p_security_engine->RSA_KEY_READ_DISABLE_REG = 0;
    p_security_engine->_0x0 &= 0xFFFFFFFB;
    
    /* Currently unknown what each flag does. */
    for (unsigned int i = 0; i < KEYSLOT_AES_MAX; i++) {
        set_aes_keyslot_flags(i, 0x15);
    }
    
    for (unsigned int i = 4; i < KEYSLOT_AES_MAX; i++) {
        set_aes_keyslot_flags(i, 0x40);
    }
    
    for (unsigned int i = 0; i < KEYSLOT_RSA_MAX; i++) {
        set_rsa_keyslot_flags(i, 0x41);
    }
    
    /* Detect Master Key revision. */
    mkey_detect_revision();
    
    /* Setup new device key, if necessary. */
    if (mkey_get_revision() >= MASTERKEY_REVISION_400_CURRENT) {
        const uint8_t new_devicekey_source_4x[0x10] = {0x8B, 0x4E, 0x1C, 0x22, 0x42, 0x07, 0xC8, 0x73, 0x56, 0x94, 0x08, 0x8B, 0xCC, 0x47, 0x0F, 0x5D};
        se_aes_ecb_decrypt_block(KEYSLOT_SWITCH_4XNEWDEVICEKEYGENKEY, work_buffer, 0x10, new_devicekey_source_4x, 0x10);
        decrypt_data_into_keyslot(KEYSLOT_SWITCH_DEVICEKEY, KEYSLOT_SWITCH_4XNEWCONSOLEKEYGENKEY, work_buffer, 0x10);
        clear_aes_keyslot(KEYSLOT_SWITCH_4XNEWCONSOLEKEYGENKEY);
        set_aes_keyslot_flags(KEYSLOT_SWITCH_DEVICEKEY, 0xFF);
    }
    
    se_initialize_rng(KEYSLOT_SWITCH_DEVICEKEY);
    
    /* Generate random data, transform with device key to get RNG key. */
    se_generate_random(KEYSLOT_SWITCH_DEVICEKEY, work_buffer, 0x10);
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_RNGKEY, KEYSLOT_SWITCH_DEVICEKEY, work_buffer, 0x10);
    set_aes_keyslot_flags(KEYSLOT_SWITCH_RNGKEY, 0xFF);
    
    /* Repeat for Session key. */
    se_generate_random(KEYSLOT_SWITCH_DEVICEKEY, work_buffer, 0x10);
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_SESSIONKEY, KEYSLOT_SWITCH_DEVICEKEY, work_buffer, 0x10);
    set_aes_keyslot_flags(KEYSLOT_SWITCH_SESSIONKEY, 0xFF);
    
    /* TODO: Create Test Vector, to validate keyslot data is unchanged post warmboot. */
}

void setup_boot_config(void) {
    /* Load boot config only if dev unit. */
    if (configitem_is_retail()) {
        bootconfig_clear();
    } else {
        flush_dcache_range((uint8_t *)NX_BOOTLOADER_BOOTCONFIG_POINTER, (uint8_t *)NX_BOOTLOADER_BOOTCONFIG_POINTER + sizeof(bootconfig_t));
        bootconfig_load_and_verify((bootconfig_t *)NX_BOOTLOADER_BOOTCONFIG_POINTER);
    }
}

bool rsa2048_pss_verify(const void *signature, size_t signature_size, const void *modulus, size_t modulus_size, const void *data, size_t data_size) {
    uint8_t message[RSA_2048_BYTES];
    uint8_t h_buf[0x24];
    
    /* Hardcode RSA with keyslot 0. */
    const uint8_t public_exponent[4] = {0x00, 0x01, 0x00, 0x01};
    set_rsa_keyslot(0, modulus, modulus_size, public_exponent, sizeof(public_exponent));
    se_synchronous_exp_mod(0, message, sizeof(message), signature, signature_size);
    
    
    /* Validate sanity byte. */
    if (message[RSA_2048_BYTES - 1] != 0xBC) {
        return false;
    }
    
    /* Copy Salt into MGF1 Hash Buffer. */
    memset(h_buf, 0, sizeof(h_buf));
    memcpy(h_buf, message + RSA_2048_BYTES - 0x20 - 0x1, 0x20);
    
    /* Decrypt maskedDB (via inline MGF1). */
    uint8_t seed = 0;
    uint8_t mgf1_buf[0x20];
    for (unsigned int ofs = 0; ofs < RSA_2048_BYTES - 0x20 - 1; ofs += 0x20) {
        h_buf[sizeof(h_buf) - 1] = seed++;
        flush_dcache_range(h_buf, h_buf + sizeof(h_buf));
        se_calculate_sha256(mgf1_buf, h_buf, sizeof(h_buf));
        for (unsigned int i = ofs; i < ofs + 0x20 && i < RSA_2048_BYTES - 0x20 - 1; i++) {
            message[i] ^= mgf1_buf[i - ofs];
        }
    }
    
    /* Constant lmask for rsa-2048-pss. */
    message[0] &= 0x7F; 

    /* Validate DB is of the form 0000...0001. */
    for (unsigned int i = 0; i < RSA_2048_BYTES - 0x20 - 0x20 - 1 - 1; i++) {
        if (message[i] != 0) {
            return false;
        }
    }   
    if (message[RSA_2048_BYTES - 0x20 - 0x20 - 1 - 1] != 1) {
        return false;
    }

    /* Check hash correctness. */
    uint8_t validate_buf[8 + 0x20 + 0x20];
    uint8_t validate_hash[0x20];
    
    memset(validate_buf, 0, sizeof(validate_buf));
    flush_dcache_range((uint8_t *)data, (uint8_t *)data + data_size);
    se_calculate_sha256(&validate_buf[8], data, data_size);
    memcpy(&validate_buf[0x28], &m_buf[RSA_2048_BYTES - 0x20 - 0x20 - 1], 0x20);
    flush_dcache_range(validate_buf, validate_buf + sizeof(validate_buf));
    se_calculate_sha256(validate_hash, validate_buf, sizeof(validate_buf));
    return memcmp(h_buf, validate_hash, 0x20) == 0;
}

void package2_crypt_ctr(unsigned int master_key_rev, void *dst, size_t dst_size, const void *src, size_t src_size, const void *ctr, size_t ctr_size) {
    /* Derive package2 key. */
    const uint8_t package2_key_source[0x10] = {0xFB, 0x8B, 0x6A, 0x9C, 0x79, 0x00, 0xC8, 0x49, 0xEF, 0xD2, 0x4D, 0x85, 0x4D, 0x30, 0xA0, 0xC7};
    flush_dcache_range((uint8_t *)dst, (uint8_t *)dst + dst_size);
    flush_dcache_range((uint8_t *)src, (uint8_t *)src + src_size);
    unsigned int keyslot = mkey_get_keyslot(master_key_rev);
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_PACKAGE2KEY, keyslot, package2_key_source, 0x10);
    
    /* Perform Encryption. */
    se_aes_ctr_crypt(KEYSLOT_SWITCH_PACKAGE2KEY, dst, dst_size, src, src_size, ctr, ctr_size);
}


void verify_header_signature(package2_header_t *header) {
    const uint8_t *modulus;

    if (configitem_is_retail()) {
        const uint8_t package2_modulus_retail[0x100] = {
            0x8D, 0x13, 0xA7, 0x77, 0x6A, 0xE5, 0xDC, 0xC0, 0x3B, 0x25, 0xD0, 0x58, 0xE4, 0x20, 0x69, 0x59,
            0x55, 0x4B, 0xAB, 0x70, 0x40, 0x08, 0x28, 0x07, 0xA8, 0xA7, 0xFD, 0x0F, 0x31, 0x2E, 0x11, 0xFE,
            0x47, 0xA0, 0xF9, 0x9D, 0xDF, 0x80, 0xDB, 0x86, 0x5A, 0x27, 0x89, 0xCD, 0x97, 0x6C, 0x85, 0xC5,
            0x6C, 0x39, 0x7F, 0x41, 0xF2, 0xFF, 0x24, 0x20, 0xC3, 0x95, 0xA6, 0xF7, 0x9D, 0x4A, 0x45, 0x74,
            0x8B, 0x5D, 0x28, 0x8A, 0xC6, 0x99, 0x35, 0x68, 0x85, 0xA5, 0x64, 0x32, 0x80, 0x9F, 0xD3, 0x48,
            0x39, 0xA2, 0x1D, 0x24, 0x67, 0x69, 0xDF, 0x75, 0xAC, 0x12, 0xB5, 0xBD, 0xC3, 0x29, 0x90, 0xBE,
            0x37, 0xE4, 0xA0, 0x80, 0x9A, 0xBE, 0x36, 0xBF, 0x1F, 0x2C, 0xAB, 0x2B, 0xAD, 0xF5, 0x97, 0x32,
            0x9A, 0x42, 0x9D, 0x09, 0x8B, 0x08, 0xF0, 0x63, 0x47, 0xA3, 0xE9, 0x1B, 0x36, 0xD8, 0x2D, 0x8A,
            0xD7, 0xE1, 0x54, 0x11, 0x95, 0xE4, 0x45, 0x88, 0x69, 0x8A, 0x2B, 0x35, 0xCE, 0xD0, 0xA5, 0x0B,
            0xD5, 0x5D, 0xAC, 0xDB, 0xAF, 0x11, 0x4D, 0xCA, 0xB8, 0x1E, 0xE7, 0x01, 0x9E, 0xF4, 0x46, 0xA3,
            0x8A, 0x94, 0x6D, 0x76, 0xBD, 0x8A, 0xC8, 0x3B, 0xD2, 0x31, 0x58, 0x0C, 0x79, 0xA8, 0x26, 0xE9,
            0xD1, 0x79, 0x9C, 0xCB, 0xD4, 0x2B, 0x6A, 0x4F, 0xC6, 0xCC, 0xCF, 0x90, 0xA7, 0xB9, 0x98, 0x47,
            0xFD, 0xFA, 0x4C, 0x6C, 0x6F, 0x81, 0x87, 0x3B, 0xCA, 0xB8, 0x50, 0xF6, 0x3E, 0x39, 0x5D, 0x4D,
            0x97, 0x3F, 0x0F, 0x35, 0x39, 0x53, 0xFB, 0xFA, 0xCD, 0xAB, 0xA8, 0x7A, 0x62, 0x9A, 0x3F, 0xF2,
            0x09, 0x27, 0x96, 0x3F, 0x07, 0x9A, 0x91, 0xF7, 0x16, 0xBF, 0xC6, 0x3A, 0x82, 0x5A, 0x4B, 0xCF,
            0x49, 0x50, 0x95, 0x8C, 0x55, 0x80, 0x7E, 0x39, 0xB1, 0x48, 0x05, 0x1E, 0x21, 0xC7, 0x24, 0x4F
        };
        modulus = package2_modulus_retail;
    } else {
        const uint8_t package2_modulus_dev[0x100] = {
            0xB3, 0x65, 0x54, 0xFB, 0x0A, 0xB0, 0x1E, 0x85, 0xA7, 0xF6, 0xCF, 0x91, 0x8E, 0xBA, 0x96, 0x99,
            0x0D, 0x8B, 0x91, 0x69, 0x2A, 0xEE, 0x01, 0x20, 0x4F, 0x34, 0x5C, 0x2C, 0x4F, 0x4E, 0x37, 0xC7,
            0xF1, 0x0B, 0xD4, 0xCD, 0xA1, 0x7F, 0x93, 0xF1, 0x33, 0x59, 0xCE, 0xB1, 0xE9, 0xDD, 0x26, 0xE6,
            0xF3, 0xBB, 0x77, 0x87, 0x46, 0x7A, 0xD6, 0x4E, 0x47, 0x4A, 0xD1, 0x41, 0xB7, 0x79, 0x4A, 0x38,
            0x06, 0x6E, 0xCF, 0x61, 0x8F, 0xCD, 0xC1, 0x40, 0x0B, 0xFA, 0x26, 0xDC, 0xC0, 0x34, 0x51, 0x83,
            0xD9, 0x3B, 0x11, 0x54, 0x3B, 0x96, 0x27, 0x32, 0x9A, 0x95, 0xBE, 0x1E, 0x68, 0x11, 0x50, 0xA0,
            0x6B, 0x10, 0xA8, 0x83, 0x8B, 0xF5, 0xFC, 0xBC, 0x90, 0x84, 0x7A, 0x5A, 0x5C, 0x43, 0x52, 0xE6,
            0xC8, 0x26, 0xE9, 0xFE, 0x06, 0xA0, 0x8B, 0x53, 0x0F, 0xAF, 0x1E, 0xC4, 0x1C, 0x0B, 0xCF, 0x50,
            0x1A, 0xA4, 0xF3, 0x5C, 0xFB, 0xF0, 0x97, 0xE4, 0xDE, 0x32, 0x0A, 0x9F, 0xE3, 0x5A, 0xAA, 0xB7,
            0x44, 0x7F, 0x5C, 0x33, 0x60, 0xB9, 0x0F, 0x22, 0x2D, 0x33, 0x2A, 0xE9, 0x69, 0x79, 0x31, 0x42,
            0x8F, 0xE4, 0x3A, 0x13, 0x8B, 0xE7, 0x26, 0xBD, 0x08, 0x87, 0x6C, 0xA6, 0xF2, 0x73, 0xF6, 0x8E,
            0xA7, 0xF2, 0xFE, 0xFB, 0x6C, 0x28, 0x66, 0x0D, 0xBD, 0xD7, 0xEB, 0x42, 0xA8, 0x78, 0xE6, 0xB8,
            0x6B, 0xAE, 0xC7, 0xA9, 0xE2, 0x40, 0x6E, 0x89, 0x20, 0x82, 0x25, 0x8E, 0x3C, 0x6A, 0x60, 0xD7,
            0xF3, 0x56, 0x8E, 0xEC, 0x8D, 0x51, 0x8A, 0x63, 0x3C, 0x04, 0x78, 0x23, 0x0E, 0x90, 0x0C, 0xB4,
            0xE7, 0x86, 0x3B, 0x4F, 0x8E, 0x13, 0x09, 0x47, 0x32, 0x0E, 0x04, 0xB8, 0x4D, 0x5B, 0xB0, 0x46,
            0x71, 0xB0, 0x5C, 0xF4, 0xAD, 0x63, 0x4F, 0xC5, 0xE2, 0xAC, 0x1E, 0xC4, 0x33, 0x96, 0x09, 0x7B
        };
        modulus = package2_modulus_dev;
    }
    
    /* This is normally only allowed on dev units, but we'll allow it anywhere. */
    if (bootconfig_is_package2_unsigned() == 0 && rsa2048_pss_verify(header->signature, 0x100, modulus, 0x100, header->encrypted_header, 0x100) == 0) {
        panic();
    }
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
        void *section_data = (void *)((uint8_t *)NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS + sizeof(package2_header_t) + metadata->section_offsets[section]);
        uint8_t calculated_hash[0x20];
        se_calculate_sha256(calculated_hash, section_data, metadata->section_sizes[section]);
        if (memcmp(calculated_hash, metadata->section_hashes[section], sizeof(metadata->section_hashes[section])) != 0) {
            return false;
        }
    }

    /* Ensure that entrypoint is present in one of our sections. */
    if (!entrypoint_found) {
        return false;
    }

    /* Perform version checks. */
    /* We will be compatible with all package2s released before current, but not newer ones. */
    if (metadata->version_max >= PACKAGE2_MINVER_THEORETICAL && metadata->version_min < PACKAGE2_MAXVER_400_CURRENT) {
        return false;
    }

    return true;
}

/* Decrypts package2 header, and returns the master key revision required. */
uint32_t decrypt_and_validate_header(package2_header_t *header) {
    package2_meta_t metadata;
    
    if (bootconfig_is_package2_plaintext() == 0) {
        uint32_t mkey_rev;
        
        /* Try to decrypt for all possible master keys. */
        for (mkey_rev = 0; mkey_rev < MASTERKEY_REVISION_MAX; mkey_rev++) {
            package2_crypt_ctr(mkey_rev, &metadata, sizeof(package2_meta_t), &header->metadata, sizeof(package2_meta_t), header->metadata.ctr, sizeof(header->metadata.ctr));
            /* Copy the ctr (which stores information) into the decrypted metadata. */
            memcpy(metadata.ctr, header->metadata.ctr, sizeof(header->metadata.ctr));
            /* See if this is the correct key. */
            if (validate_package2_metadata(&metadata)) {
                memcpy(&header->metadata, metadata, sizeof(package2_meta_t));
                break;
            }
        }
            
        /* Ensure we successfully decrypted the header. */
        panic();  
    }
    return 0;
}

void load_package2_sections(package2_meta_t *metadata, uint32_t master_key_rev) {
    /* By default, copy data directly from where NX_BOOTLOADER puts it. */
    void *load_buf = NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS;
    
    /* Check whether any of our sections overlap this region. If they do, we must relocate and copy from elsewhere. */
    bool needs_relocation = false;
    for (unsigned int section = 0; section < PACKAGE2_SECTION_MAX; section++) {
        uint64_t section_start = DRAM_BASE_PHYSICAL + (uint64_t)metadata->section_offsets[section];
        uint64_t section_end = section_start + (uint64_t)metadata->section_sizes[section];
        if (overlaps(section_start, section_end, (uint64_t)(NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS), (uint64_t)(NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS) + PACKAGE2_SIZE_MAX)) {
            needs_relocation = true;
        }
    }
    if (needs_relocation) {
        /* This code should *always* succeed in finding a carveout within seven loops, */
        /* due to the section size limit, and section number limit. */
        /* However, Nintendo tries panics after 8 loops if a safe section is not found. */
        /* This should never be the case, mathematically. */
        /* We will replicate this behavior. */
        bool found_safe_carveout = false;
        uint64_t potential_base_start = DRAM_BASE_PHYSICAL;
        uint64_t potential_base_end = potential_base_start + PACKAGE2_SIZE_MAX;
        for (unsigned int i = 0; i < 8; i++) {
            int is_safe = 1;
            for (unsigned int section = 0; section < PACKAGE2_SECTION_MAX; section++) {
                uint64_t section_start = DRAM_BASE_PHYSICAL + (uint64_t)metadata->section_offsets[section];
                uint64_t section_end = section_start + (uint64_t)metadata->section_sizes[section];
                if (overlaps(section_start, section_end, potential_base_start, potential_base_end)) {
                    is_safe = 0;
                }
            }
            found_safe_carveout |= is_safe;
            if (found_safe_carveout) {
                break;
            }
            potential_base_start += PACKAGE2_SIZE_MAX;
            potential_base_end += PACKAGE2_SIZE_MAX;
        }
        if (!found_safe_carveout) {
            panic();
        }
        /* Relocate to new carveout. */
        memcpy((void *)potential_base_start, load_buf, PACKAGE2_SIZE_MAX);
        memset(load_buf, 0, PACKAGE2_SIZE_MAX);
        load_buf = (void *)potential_base_start;
    }
    
    /* Copy each section to its appropriate location, decrypting if necessary. */
    for (unsigned int section = 0; section < PACKAGE2_SECTION_MAX; section++) {        
        if (metadata->section_sizes[section] == 0) {
            continue;
        }
        
        void *dst_start = (void *)(DRAM_BASE_PHYSICAL + (uint64_t)metadata->section_offsets[section]);
        void *src_start = load_buf + sizeof(package2_header_t) + metadata->section_offsets[section];
        size_t size = (size_t)metadata->section_sizes[section];
        
        if (bootconfig_is_package2_plaintext()) {
            memcpy(dst_start, src_start, size);
        } else {
            package2_crypt_ctr(master_key_rev, dst_start, size, src_start, size, metadata->section_ctrs[section], 0x10);
        }
    }
    
    /* Clear the encrypted package2 from memory. */
    memset(load_buf, 0, PACKAGE2_SIZE_MAX);
}

/* This function is called during coldboot crt0, and validates a package2. */
/* This package2 is read into memory by a concurrent BPMP bootloader. */
void load_package2(void) {
    /* Setup MMIO virtual pointers. */
    setup_mmio_virtual_addresses();
    
    /* Setup the Security Engine. */
    setup_se();
    
    /* TODO: bootup_misc_mmio(). */
    /* This func will also be called on warmboot. */
    /* And will verify stored SE Test Vector, clear keyslots, */
    /* Generate an SRK, set the warmboot firmware location, */
    /* Configure the GPU uCode carveout, configure the Kernel default carveouts, */
    /* Initialize the PMC secure scratch registers, initialize MISC registers, */
    /* And assign "se_operation_completed" to Interrupt 0x5A. */
    
    /* TODO: Read and save BOOTREASON stored by NX_BOOTLOADER at 0x1F009FE00 */
    
    /* Initialize cache'd random bytes for kernel. */
    randomcache_init();
    
    /* TODO: memclear the initial copy of Exosphere running in IRAM (relocated to TZRAM by earlier code). */
    
    /* Let NX Bootloader know that we're running. */
    MAILBOX_NX_BOOTLOADER_IS_SECMON_AWAKE = 1;
    
    /* Synchronize with NX BOOTLOADER. */
    if (MAILBOX_NX_BOOTLOADER_SETUP_STATE == NX_BOOTLOADER_STATE_INIT) {
        while (MAILBOX_NX_BOOTLOADER_SETUP_STATE < NX_BOOTLOADER_STATE_MOVED_BOOTCONFIG) {
            wait(1);
        }
    }

    /* Load Boot Config into global. */
    setup_boot_config();
    
    /* Synchronize with NX BOOTLOADER. */
    if (MAILBOX_NX_BOOTLOADER_SETUP_STATE == NX_BOOTLOADER_STATE_MOVED_BOOTCONFIG) {
        while (MAILBOX_NX_BOOTLOADER_SETUP_STATE < NX_BOOTLOADER_STATE_LOADED_PACKAGE2) {
            wait(1);
        }
    }
    
    /* Load header from NX_BOOTLOADER-initialized DRAM. */
    package2_header_t header;
    flush_dcache_range((uint8_t *)NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS, (uint8_t *)NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS + sizeof(header));
    memcpy(&header, NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS, sizeof(header));
    flush_dcache_range((uint8_t *)&header, (uint8_t *)&header + sizeof(header));
    
    /* Perform signature checks. */
    verify_header_signature(&header);
    
    /* Decrypt header, get key revision required. */
    uint32_t package2_mkey_rev = decrypt_and_validate_header(&header);
        
    /* Load Package2 Sections. */
    load_package2_sections(&header->metadata, package2_mkey_rev);
    
    /* Clean up cache. */
    flush_dcache_all();
    invalidate_icache_inner_shareable();
    
    /* Set CORE0 entrypoint for Package2. */
    set_core_entrypoint_and_context_id(0, DRAM_BASE_PHYSICAL + header.metadata.entrypoint, 0);
    
    /* TODO: Nintendo clears 0x1F01FA7D0 to 0x1F01FA7E8. What does this do? Does it remove the identity mapping page tables? */ 
    tlb_invalidate_all();
    
    /* Synchronize with NX BOOTLOADER. */
    if (MAILBOX_NX_BOOTLOADER_SETUP_STATE == NX_BOOTLOADER_STATE_LOADED_PACKAGE2) {
        while (MAILBOX_NX_BOOTLOADER_SETUP_STATE < NX_BOOTLOADER_STATE_FINISHED) {
            wait(1);
        }
    }
    
    /* TODO: MISC register 0x1F0098C00 |= 0x2000;
    
    /* TODO: Update SCR_EL3 depending on value in Bootconfig. */     
}