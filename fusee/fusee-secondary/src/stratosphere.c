/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <sys/stat.h>
#include "exocfg.h"
#include "utils.h"
#include "package2.h"
#include "stratosphere.h"
#include "fs_utils.h"
#include "ips.h"
#include "lib/log.h"

#define u8 uint8_t
#define u32 uint32_t
#include "loader_kip.h"
#include "pm_kip.h"
#include "sm_kip.h"
#include "ams_mitm_kip.h"
#include "boot_100_kip.h"
#include "boot_200_kip.h"
#include "spl_kip.h"
#undef u8
#undef u32

static ini1_header_t *g_stratosphere_ini1 = NULL;
static ini1_header_t *g_sd_files_ini1 = NULL;

static bool g_stratosphere_loader_enabled = true;
static bool g_stratosphere_sm_enabled = true;
static bool g_stratosphere_pm_enabled = true;
static bool g_stratosphere_ams_mitm_enabled = true;
static bool g_stratosphere_spl_enabled = true;
static bool g_stratosphere_boot_enabled = false;

extern const uint8_t boot_100_kip[], boot_200_kip[];
extern const uint8_t loader_kip[], pm_kip[], sm_kip[], spl_kip[], ams_mitm_kip[];
extern const uint32_t boot_100_kip_size, boot_200_kip_size;
extern const uint32_t loader_kip_size, pm_kip_size, sm_kip_size, spl_kip_size, ams_mitm_kip_size;

/* GCC doesn't consider the size as const... we have to write it ourselves. */

ini1_header_t *stratosphere_get_ini1(uint32_t target_firmware) {
    const uint8_t *boot_kip = NULL;
    uint32_t boot_kip_size = 0;
    uint32_t num_processes = 0;
    uint8_t *data;

    if (g_stratosphere_ini1 != NULL) {
        return g_stratosphere_ini1;
    }

    if (target_firmware <= ATMOSPHERE_TARGET_FIRMWARE_100) {
        boot_kip = boot_100_kip;
        boot_kip_size = boot_100_kip_size;
    } else {
        boot_kip = boot_200_kip;
        boot_kip_size = boot_200_kip_size;
    }

    size_t size = sizeof(ini1_header_t);
    
    /* Calculate our processes' sizes. */
    if (g_stratosphere_loader_enabled) {
        size += loader_kip_size;
        num_processes++;
    }
    
    if (g_stratosphere_pm_enabled) {
        size += pm_kip_size;
        num_processes++;
    }
    
    if (g_stratosphere_sm_enabled) {
        size += sm_kip_size;
        num_processes++;
    }
    
    if (g_stratosphere_spl_enabled) {
        size += spl_kip_size;
        num_processes++;
    }
    
    if (g_stratosphere_ams_mitm_enabled) {
        size += ams_mitm_kip_size;
        num_processes++;
    }
    
    if (g_stratosphere_boot_enabled) {
        size += boot_kip_size;
        num_processes++;
    }
    
    g_stratosphere_ini1 = (ini1_header_t *)malloc(size);

    if (g_stratosphere_ini1 == NULL) {
        fatal_error("stratosphere_get_ini1: out of memory!\n");
    }

    g_stratosphere_ini1->magic = MAGIC_INI1;
    g_stratosphere_ini1->size = size;
    g_stratosphere_ini1->num_processes = num_processes;
    g_stratosphere_ini1->_0xC = 0;

    data = g_stratosphere_ini1->kip_data;

    /* Copy our processes. */
    if (g_stratosphere_loader_enabled) {
        memcpy(data, loader_kip, loader_kip_size);
        data += loader_kip_size;
    }

    if (g_stratosphere_pm_enabled) {
        memcpy(data, pm_kip, pm_kip_size);
        data += pm_kip_size;
    }

    if (g_stratosphere_sm_enabled) {
        memcpy(data, sm_kip, sm_kip_size);
        data += sm_kip_size;
    }

    if (g_stratosphere_spl_enabled) {
        memcpy(data, spl_kip, spl_kip_size);
        data += spl_kip_size;
    }

    if (g_stratosphere_ams_mitm_enabled) {
        memcpy(data, ams_mitm_kip, ams_mitm_kip_size);
        data += ams_mitm_kip_size;
    }

    if (g_stratosphere_boot_enabled) {
        memcpy(data, boot_kip, boot_kip_size);
        data += boot_kip_size;
    }
    
    return g_stratosphere_ini1;
}

void stratosphere_free_ini1(void) {
    if (g_stratosphere_ini1 != NULL) {
        free(g_stratosphere_ini1);
        g_stratosphere_ini1 = NULL;
    }
    if (g_sd_files_ini1 != NULL) {
        free(g_sd_files_ini1);
        g_sd_files_ini1 = NULL;
    }
}



static void try_add_sd_kip(ini1_header_t *ini1, const char *kip_path) {
    size_t file_size = get_file_size(kip_path);
    
    if (ini1->size + file_size > PACKAGE2_SIZE_MAX) {
        fatal_error("Failed to load %s: INI1 would be too large!\n", kip_path);
    }
    
    kip1_header_t kip_header;
    if (read_from_file(&kip_header, sizeof(kip_header), kip_path) != sizeof(kip_header) || kip_header.magic != MAGIC_KIP1) {
        return;
    }
    
    size_t kip_size = kip1_get_size_from_header(&kip_header);
    if (kip_size > file_size) {
        fatal_error("Failed to load %s: KIP size is corrupt!\n", kip_path);
    }
    
    if (read_from_file(ini1->kip_data + ini1->size - sizeof(ini1_header_t), kip_size, kip_path) != kip_size) {
        /* TODO: is this error fatal? */
        return;
    }
    
    ini1->size += kip_size;
    ini1->num_processes++;
}

ini1_header_t *stratosphere_get_sd_files_ini1(void) {    
    if (g_sd_files_ini1 != NULL) {
        return g_sd_files_ini1;
    }
    
    /* Allocate space. */
    g_sd_files_ini1 = (ini1_header_t *)malloc(PACKAGE2_SIZE_MAX);
    g_sd_files_ini1->magic = MAGIC_INI1;
    g_sd_files_ini1->size = sizeof(ini1_header_t);
    g_sd_files_ini1->num_processes = 0;
    g_sd_files_ini1->_0xC = 0;
    
    /* Load all kips from /atmosphere/kips/<*>.kip or /atmosphere/kips/<*>/<*>.kip. */
    DIR *kips_dir = opendir("atmosphere/kips");
    struct dirent *ent;
    if (kips_dir != NULL) {
        while ((ent = readdir(kips_dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            
            char kip_path[0x301] = {0};
            snprintf(kip_path, 0x300, "atmosphere/kips/%s", ent->d_name);
            
            struct stat kip_stat;
            if (stat(kip_path, &kip_stat) == -1) {
                continue;
            }
            
            if ((kip_stat.st_mode & S_IFMT) == S_IFREG) {
                /* If file, add to ini1. */
                try_add_sd_kip(g_sd_files_ini1, kip_path);
            } else if ((kip_stat.st_mode & S_IFMT) == S_IFDIR) {
                /* Otherwise, allow one level of nesting. */
                DIR *sub_dir = opendir(kip_path);
                struct dirent *sub_ent;
                if (sub_dir != NULL) {
                    while ((sub_ent = readdir(sub_dir)) != NULL) {
                        if (strcmp(sub_ent->d_name, ".") == 0 || strcmp(sub_ent->d_name, "..") == 0) {
                            continue;
                        }
                        
                        /* Reuse kip path variable. */
                        memset(kip_path, 0, sizeof(kip_path));
                        snprintf(kip_path, 0x300, "atmosphere/kips/%s/%s", ent->d_name, sub_ent->d_name);
                        
                        if (stat(kip_path, &kip_stat) == -1) {
                            continue;
                        }
                        
                        if ((kip_stat.st_mode & S_IFMT) == S_IFREG) {
                            /* If file, add to ini1. */
                            try_add_sd_kip(g_sd_files_ini1, kip_path);
                        }
                    }
                    closedir(sub_dir);
                }
            }
        }
        closedir(kips_dir);
    }
    
    return g_sd_files_ini1;
}

/* Merges some number of INI1s into a single INI1. It's assumed that the INIs are in order of preference. */
ini1_header_t *stratosphere_merge_inis(ini1_header_t **inis, size_t num_inis) {
    uint32_t total_num_processes = 0;

    /* Validate all ini headers. */
    for (size_t i = 0; i < num_inis; i++) {
        if (inis[i] == NULL || inis[i]->magic != MAGIC_INI1 || inis[i]->num_processes > INI1_MAX_KIPS) {
            fatal_error("INI1s[%d] section appears to not contain an INI1!\n", i);
        } else {
            total_num_processes += inis[i]->num_processes;
        }
    }

    if (total_num_processes > INI1_MAX_KIPS) {
        fatal_error("The resulting INI1 would have too many KIPs!\n");
    }

    uint64_t process_list[INI1_MAX_KIPS] = {0};
    ini1_header_t *merged = (ini1_header_t *)malloc(PACKAGE2_SIZE_MAX); /* because of SD file overrides */

    if (merged == NULL) {
        fatal_error("stratosphere_merge_inis: out of memory!\n");
    }

    merged->magic = MAGIC_INI1;
    merged->num_processes = 0;
    merged->_0xC = 0;
    size_t remaining_size = PACKAGE2_SIZE_MAX - sizeof(ini1_header_t);

    unsigned char *current_dst_kip = merged->kip_data;

    /* Actually merge into the inis. */
    for (size_t i = 0; i < num_inis; i++) {
        size_t offset = 0;
        for (size_t p = 0; p < (size_t)inis[i]->num_processes; p++) {
            kip1_header_t *current_kip = (kip1_header_t *)(inis[i]->kip_data + offset);
            if (current_kip->magic != MAGIC_KIP1) {
                fatal_error("INI1s[%zu][%zu] appears not to be a KIP1!\n", i, p);
            }

            offset += kip1_get_size_from_header(current_kip);
            
            bool already_loaded = false;
            for (uint32_t j = 0; j < merged->num_processes; j++) {
                if (process_list[j] == current_kip->title_id) {
                    already_loaded = true;
                    break;
                }
            }
            if (already_loaded) {
                continue;
            }
            
            print(SCREEN_LOG_LEVEL_MANDATORY, "[NXBOOT]: Loading KIP %08x%08x...\n", (uint32_t)(current_kip->title_id >> 32), (uint32_t)current_kip->title_id);

            size_t current_kip_size = kip1_get_size_from_header(current_kip);
            if (current_kip_size > remaining_size) {
                fatal_error("Not enough space for all the KIP1s!\n");
            }
            
            kip1_header_t *patched_kip = apply_kip_ips_patches(current_kip, current_kip_size);
            if (patched_kip != NULL) {
                size_t patched_kip_size = kip1_get_size_from_header(patched_kip);
                if (patched_kip_size > remaining_size) {
                    fatal_error("Not enough space for all the KIP1s!\n");
                }
                memcpy(current_dst_kip, patched_kip, patched_kip_size);
                remaining_size -= patched_kip_size;
                current_dst_kip += patched_kip_size;
                
                free(patched_kip);
            } else {   
                memcpy(current_dst_kip, current_kip, current_kip_size);
                remaining_size -= current_kip_size;
                current_dst_kip += current_kip_size;
            }

            process_list[merged->num_processes++] = current_kip->title_id;
        }
    }
    merged->size = sizeof(ini1_header_t) + (uint32_t)(current_dst_kip - merged->kip_data);

    return merged;
}
