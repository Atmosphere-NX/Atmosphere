/*
 * Copyright (c) 2018 Atmosphère-NX
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
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "se.h"
#include "lib/log.h"
#include "ips.h"

/* IPS Patching adapted from Luma3DS (https://github.com/AuroraWright/Luma3DS/blob/master/sysmodules/loader/source/patcher.c) */

#define IPS_MAGIC "PATCH"
#define IPS_TAIL "EOF"

#define IPS32_MAGIC "IPS32"
#define IPS32_TAIL "EEOF"

#define NOGC_PATCH_DIR "default_nogc"
static bool g_enable_nogc_patches = false;

void kip_patches_set_enable_nogc(void) {
    g_enable_nogc_patches = true;
}

static bool should_ignore_default_patch(const char *patch_dir) {
    /* This function will ensure that select default patches only get loaded if enabled. */
    if (!g_enable_nogc_patches && strcmp(patch_dir, NOGC_PATCH_DIR) == 0) {
        return true;
    }
    
    return false;
}

static bool has_patch(const char *dir, const char *subdir, const void *hash, size_t hash_size) {
    char path[0x301] = {0};
    int cur_len = 0;
    cur_len += snprintf(path + cur_len, sizeof(path) - cur_len, "%s/", dir);
    if (subdir != NULL) {
        cur_len += snprintf(path + cur_len, sizeof(path) - cur_len, "%s/", subdir);
    }
    for (size_t i = 0; i < hash_size; i++) {
         cur_len += snprintf(path + cur_len, sizeof(path) - cur_len, "%02X", ((const uint8_t *)hash)[i]);
    }
    cur_len += snprintf(path + cur_len, sizeof(path) - cur_len, ".ips");
    if (cur_len >= sizeof(path)) {
        return false;
    }
    
    FILE *f = fopen(path, "rb");
    if (f != NULL) {
        fclose(f);
        return true;
    }
    return false;
}

static bool has_needed_default_kip_patches(uint64_t title_id, const void *hash, size_t hash_size) {
    if (title_id == 0x0100000000000000ULL && g_enable_nogc_patches) {
        return has_patch("atmosphere/kip_patches", NOGC_PATCH_DIR, hash, hash_size);
    }
    
    return true;
}

/* Applies an IPS/IPS32 patch to memory, disregarding writes to the first prot_size bytes. */
static void apply_ips_patch(uint8_t *mem, size_t mem_size, size_t prot_size, bool is_ips32, FILE *f_ips) {
    uint8_t buffer[4];
    while (fread(buffer, is_ips32 ? 4 : 3, 1, f_ips) == 1) {
        if (is_ips32 && memcmp(buffer, IPS32_TAIL, 4) == 0) {
            break;
        } else if (!is_ips32 && memcmp(buffer, IPS_TAIL, 3) == 0) {
            break;
        }
        
        /* Offset of patch. */
        uint32_t patch_offset;
        if (is_ips32) {
           patch_offset = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
        } else {
           patch_offset = (buffer[0] << 16) | (buffer[1] << 8) | (buffer[2]);
        }
        
        /* Size of patch. */
        if (fread(buffer, 2, 1, f_ips) != 1) {
            break;
        }
        uint32_t patch_size = (buffer[0] << 8) | (buffer[1]);
        
        /* Check for RLE encoding. */
        if (patch_size == 0) {
            /* Size of RLE. */
            if (fread(buffer, 2, 1, f_ips) != 1) {
                break;
            }
            
            uint32_t rle_size = (buffer[0] << 8) | (buffer[1]);
            
            /* Value for RLE. */
            if (fread(buffer, 1, 1, f_ips) != 1) {
                break;
            }
            
            if (patch_offset < prot_size) {
                if (patch_offset + rle_size > prot_size) {
                    uint32_t diff = prot_size - patch_offset;
                    patch_offset += diff;
                    rle_size -= diff;
                    goto IPS_RLE_PATCH_OFFSET_WITHIN_BOUNDS;
                }
            } else {
                IPS_RLE_PATCH_OFFSET_WITHIN_BOUNDS:
                if (patch_offset + rle_size > mem_size) {
                    rle_size = mem_size - patch_offset;
                }
                memset(mem + patch_offset, buffer[0], rle_size);
            }
        } else {
            uint32_t read_size;
            if (patch_offset < prot_size) {
                if (patch_offset + patch_size > prot_size) {
                    uint32_t diff = prot_size - patch_offset;
                    patch_offset += diff;
                    patch_size -= diff;
                    fseek(f_ips, diff, SEEK_CUR);
                    goto IPS_DATA_PATCH_OFFSET_WITHIN_BOUNDS;
                } else {
                    fseek(f_ips, patch_size, SEEK_CUR);
                }
            } else {
                IPS_DATA_PATCH_OFFSET_WITHIN_BOUNDS:
                read_size = patch_size;
                if (patch_offset + read_size > mem_size) {
                    read_size = mem_size - patch_offset;
                }
                if (fread(mem + patch_offset, read_size, 1, f_ips) != 1) {
                    break;
                }
                if (patch_size > read_size) {
                    fseek(f_ips, patch_size - read_size, SEEK_CUR);
                }
            }
        }
    }
}


static inline uint8_t hex_nybble_to_u8(const char nybble) {
    if ('0' <= nybble && nybble <= '9') {
        return nybble - '0';
    } else if ('a' <= nybble && nybble <= 'f') {
        return nybble - 'a' + 0xa;
    } else {
        return nybble - 'A' + 0xA;
    }
}

static bool name_matches_hash(const char *name, size_t name_len, const void *hash, size_t hash_size) {
    /* Validate name is hex build id. */
    for (unsigned int i = 0; i < name_len - 4; i++) {
        if (isxdigit(name[i]) == 0) {
                return false;
        }
    }

    /* Read hash from name. */
    uint8_t hash_from_name[0x20] = {0};
    for (unsigned int name_ofs = 0, id_ofs = 0; name_ofs < name_len - 4; id_ofs++) {
        hash_from_name[id_ofs] |= hex_nybble_to_u8(name[name_ofs++]) << 4;
        hash_from_name[id_ofs] |= hex_nybble_to_u8(name[name_ofs++]);
    }
    
    return memcmp(hash, hash_from_name, hash_size) == 0;

}

static bool has_ips_patches(const char *dir, const void *hash, size_t hash_size) {
    bool any_patches = false;
    char path[0x301] = {0};
    snprintf(path, sizeof(path) - 1, "%s", dir);
    DIR *patches_dir = opendir(path);
    struct dirent *pdir_ent;
    if (patches_dir != NULL) {
        /* Iterate over the patches directory to find patch subdirectories. */
        while ((pdir_ent = readdir(patches_dir)) != NULL && !any_patches) {
            if (strcmp(pdir_ent->d_name, ".") == 0 || strcmp(pdir_ent->d_name, "..") == 0) {
                continue;
            }
            
            if (should_ignore_default_patch(pdir_ent->d_name)) {
                continue;
            }
            
            snprintf(path, sizeof(path) - 1, "%s/%s", dir, pdir_ent->d_name);
            DIR *patch_dir = opendir(path);
            struct dirent *ent;
            if (patch_dir != NULL) {
                /* Iterate over the patch subdirectory to find .ips patches. */
                while ((ent = readdir(patch_dir)) != NULL && !any_patches) {
                    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                        continue;
                    }
                    
                    size_t name_len = strlen(ent->d_name);
                    if ((4 < name_len && name_len <= 0x44) && ((name_len & 1) == 0) && strcmp(ent->d_name + name_len - 4, ".ips") == 0 && name_matches_hash(ent->d_name, name_len, hash, hash_size)) {
                        snprintf(path, sizeof(path) - 1, "%s/%s/%s", dir, pdir_ent->d_name, ent->d_name);
                        FILE *f_ips = fopen(path, "rb");
                        if (f_ips != NULL) {
                            uint8_t header[5];
                            if (fread(header, 5, 1, f_ips) == 1) {
                                if (memcmp(header, IPS_MAGIC, 5) == 0 || memcmp(header, IPS32_MAGIC, 5) == 0) {
                                    any_patches = true;
                                }
                            }
                            fclose(f_ips);
                        }
                    }
                }
                closedir(patch_dir);
            }
        }
        closedir(patches_dir);
    }

    return any_patches;
}

static void apply_ips_patches(const char *dir, void *mem, size_t mem_size, size_t prot_size, const void *hash, size_t hash_size) {
    char path[0x301] = {0};
    snprintf(path, sizeof(path) - 1, "%s", dir);
    DIR *patches_dir = opendir(path);
    struct dirent *pdir_ent;
    if (patches_dir != NULL) {
        /* Iterate over the patches directory to find patch subdirectories. */
        while ((pdir_ent = readdir(patches_dir)) != NULL) {
            if (strcmp(pdir_ent->d_name, ".") == 0 || strcmp(pdir_ent->d_name, "..") == 0) {
                continue;
            }
            
            if (should_ignore_default_patch(pdir_ent->d_name)) {
                continue;
            }
            
            snprintf(path, sizeof(path) - 1, "%s/%s", dir, pdir_ent->d_name);
            DIR *patch_dir = opendir(path);
            struct dirent *ent;
            if (patch_dir != NULL) {
                /* Iterate over the patch subdirectory to find .ips patches. */
                while ((ent = readdir(patch_dir)) != NULL) {
                    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                        continue;
                    }
                    
                    size_t name_len = strlen(ent->d_name);
                    if ((4 < name_len && name_len <= 0x44) && ((name_len & 1) == 0) && strcmp(ent->d_name + name_len - 4, ".ips") == 0 && name_matches_hash(ent->d_name, name_len, hash, hash_size)) {
                        snprintf(path, sizeof(path) - 1, "%s/%s/%s", dir, pdir_ent->d_name, ent->d_name);
                        FILE *f_ips = fopen(path, "rb");
                        if (f_ips != NULL) {
                            uint8_t header[5];
                            if (fread(header, 5, 1, f_ips) == 1) {
                                if (memcmp(header, IPS_MAGIC, 5) == 0) {
                                    apply_ips_patch(mem, mem_size, prot_size, false, f_ips);
                                } else if (memcmp(header, IPS32_MAGIC, 5) == 0) {
                                    apply_ips_patch(mem, mem_size, prot_size, true, f_ips);
                                }
                            }
                            fclose(f_ips);
                        }
                    }
                }
                closedir(patch_dir);
            }
        }
        closedir(patches_dir);
    }
} 

void apply_kernel_ips_patches(void *kernel, size_t kernel_size) {
    uint8_t hash[0x20];
    se_calculate_sha256(hash, kernel, kernel_size);
    apply_ips_patches("atmosphere/kernel_patches", kernel, kernel_size, 0, hash, sizeof(hash));
}

static void kip1_blz_uncompress(void *hdr_end) {
    uint8_t *u8_hdr_end = (uint8_t *)hdr_end;
    uint32_t addl_size = ((u8_hdr_end[-4]) << 0) | ((u8_hdr_end[-3]) << 8) | ((u8_hdr_end[-2]) << 16) | ((u8_hdr_end[-1]) << 24);
    uint32_t header_size = ((u8_hdr_end[-8]) << 0) | ((u8_hdr_end[-7]) << 8) | ((u8_hdr_end[-6]) << 16) | ((u8_hdr_end[-5]) << 24);
    uint32_t cmp_and_hdr_size = ((u8_hdr_end[-12]) << 0) | ((u8_hdr_end[-11]) << 8) | ((u8_hdr_end[-10]) << 16) | ((u8_hdr_end[-9]) << 24);
        
    unsigned char *cmp_start = (unsigned char *)(((uintptr_t)hdr_end) - cmp_and_hdr_size);
    uint32_t cmp_ofs = cmp_and_hdr_size - header_size;
    uint32_t out_ofs = cmp_and_hdr_size + addl_size;
    
    while (out_ofs) {
        unsigned char control = cmp_start[--cmp_ofs];
        for (unsigned int i = 0; i < 8; i++) {
            if (control & 0x80) {
                if (cmp_ofs < 2) {      
                    fatal_error("KIP1 decompression out of bounds!\n");
                }
                cmp_ofs -= 2;
                uint16_t seg_val = ((unsigned int)cmp_start[cmp_ofs+1] << 8) | cmp_start[cmp_ofs];
                uint32_t seg_size = ((seg_val >> 12) & 0xF) + 3;
                uint32_t seg_ofs = (seg_val & 0x0FFF) + 3;
                if (out_ofs < seg_size) {
                    /* Kernel restricts segment copy to stay in bounds. */
                    seg_size = out_ofs;
                }
                out_ofs -= seg_size;
                
                for (unsigned int j = 0; j < seg_size; j++) {
                    cmp_start[out_ofs + j] = cmp_start[out_ofs + j + seg_ofs];
                }
            } else {
                /* Copy directly. */
                if (cmp_ofs < 1) {
                    fatal_error("KIP1 decompression out of bounds!\n");
                }
                cmp_start[--out_ofs] = cmp_start[--cmp_ofs];
            }
            control <<= 1;
            if (out_ofs == 0) {
                return;
            }
        }
    }
}

static kip1_header_t *kip1_uncompress(kip1_header_t *kip, size_t *size) {
    kip1_header_t new_header = *kip;
    for (unsigned int i = 0; i < 3; i++) {
        new_header.section_headers[i].compressed_size = new_header.section_headers[i].out_size;
    }
    new_header.flags &= 0xF8;
    
    *size = kip1_get_size_from_header(&new_header);
    
    unsigned char *new_kip = calloc(1, *size);
    if (new_kip == NULL) {
        return NULL;
    }
    *((kip1_header_t *)new_kip) = new_header;
    
    size_t new_offset = 0x100;
    size_t old_offset = 0x100;
    for (unsigned int i = 0; i < 3; i++) {
        memcpy(new_kip + new_offset, (unsigned char *)kip + old_offset, kip->section_headers[i].compressed_size);
        if (kip->flags & (1 << i)) {
            kip1_blz_uncompress(new_kip + new_offset + kip->section_headers[i].compressed_size);
        }
        new_offset += kip->section_headers[i].out_size;
        old_offset += kip->section_headers[i].compressed_size;
    }
    
    return (kip1_header_t *)new_kip;
}

kip1_header_t *apply_kip_ips_patches(kip1_header_t *kip, size_t kip_size) {
    uint8_t hash[0x20];
    se_calculate_sha256(hash, kip, kip_size);
    
    if (!has_needed_default_kip_patches(kip->title_id, hash, sizeof(hash))) {
        fatal_error("[NXBOOT]: Missing default patch for KIP %08x%08x...\n", (uint32_t)(kip->title_id >> 32), (uint32_t)kip->title_id);
    }
            
    if (!has_ips_patches("atmosphere/kip_patches", hash, sizeof(hash))) {
        return NULL;
    }

    print(SCREEN_LOG_LEVEL_MANDATORY, "[NXBOOT]: Patching KIP %08x%08x...\n", (uint32_t)(kip->title_id >> 32), (uint32_t)kip->title_id);
    
        
    size_t uncompressed_kip_size;
    kip1_header_t *uncompressed_kip = kip1_uncompress(kip, &uncompressed_kip_size);
    if (uncompressed_kip == NULL) {  
        return NULL;
    }
        
    apply_ips_patches("atmosphere/kip_patches", uncompressed_kip, uncompressed_kip_size, 0x100, hash, sizeof(hash));
    return uncompressed_kip;
}
