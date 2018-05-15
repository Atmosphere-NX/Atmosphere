#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "exocfg.h"
#include "utils.h"
#include "package2.h"
#include "stratosphere.h"
#include "fs_utils.h"

static ini1_header_t *g_stratosphere_ini1 = NULL;

extern const uint8_t boot_100_kip[], boot_200_kip[];
extern const uint8_t loader_kip[], pm_kip[], sm_kip[];
extern const uint32_t boot_100_kip_size, boot_200_kip_size;
extern const uint32_t loader_kip_size, pm_kip_size, sm_kip_size;

/* GCC doesn't consider the size as const... we have to write it ourselves. */

ini1_header_t *stratosphere_get_ini1(uint32_t target_firmware) {
    const uint8_t *boot_kip = NULL;
    uint32_t boot_kip_size = 0;
    uint8_t *data;

    if (g_stratosphere_ini1 != NULL) {
        return g_stratosphere_ini1;
    }

    if (target_firmware <= EXOSPHERE_TARGET_FIRMWARE_100) {
        boot_kip = boot_100_kip;
        boot_kip_size = boot_100_kip_size;
    } else {
        boot_kip = boot_200_kip;
        boot_kip_size = boot_200_kip_size;
    }

    size_t size = sizeof(ini1_header_t) + loader_kip_size + pm_kip_size + sm_kip_size + boot_kip_size;
    g_stratosphere_ini1 = (ini1_header_t *)malloc(size);

    if (g_stratosphere_ini1 != NULL) {
        printf("Error: stratosphere_get_ini1: out of memory!\n");
        generic_panic();
    }

    g_stratosphere_ini1->magic = MAGIC_INI1;
    g_stratosphere_ini1->size = size;
    g_stratosphere_ini1->num_processes = 4;
    g_stratosphere_ini1->_0xC = 0;

    data = g_stratosphere_ini1->kip_data;

    /* Copy our processes. */
    memcpy(data, loader_kip, loader_kip_size);
    data += loader_kip_size;

    memcpy(data, pm_kip, pm_kip_size);
    data += pm_kip_size;

    memcpy(data, sm_kip, sm_kip_size);
    data += sm_kip_size;

    memcpy(data, boot_kip, boot_kip_size);
    data += boot_kip_size;

    return g_stratosphere_ini1;
}

void stratosphere_free_ini1(void) {
    free(g_stratosphere_ini1);
    g_stratosphere_ini1 = NULL;
}

/* Merges some number of INI1s into a single INI1. It's assumed that the INIs are in order of preference. */
ini1_header_t *stratosphere_merge_inis(ini1_header_t **inis, size_t num_inis) {
    char sd_path[0x100] = {0};
    uint32_t total_num_processes = 0;

    /* Validate all ini headers. */
    for (size_t i = 0; i < num_inis; i++) {
        if (inis[i] == NULL || inis[i]->magic != MAGIC_INI1 || inis[i]->num_processes > INI1_MAX_KIPS) {
            printf("Error: INI1s[%d] section appears to not contain an INI1!\n", i);
            generic_panic();
        } else {
            total_num_processes += inis[i]->num_processes;
        }
    }

    if (total_num_processes > INI1_MAX_KIPS) {
        printf("Error: The resulting INI1 would have too many KIPs!\n");
        generic_panic();
    }

    uint64_t process_list[INI1_MAX_KIPS] = {0};
    ini1_header_t *merged = (ini1_header_t *)malloc(PACKAGE2_SIZE_MAX); /* because of SD file overrides */

    if (merged == NULL) {
        printf("Error: stratosphere_merge_inis: out of memory!\n");
        generic_panic();
    }

    merged->magic = MAGIC_INI1;
    merged->num_processes = 0;
    merged->_0xC = 0;
    size_t remaining_size = PACKAGE2_SIZE_MAX - sizeof(ini1_header_t);
    size_t read_size;

    unsigned char *current_dst_kip = merged->kip_data;

    /* Actually merge into the inis. */
    for (size_t i = 0; i < num_inis; i++) {
        size_t offset = 0;
        for (size_t p = 0; p < (size_t)inis[i]->num_processes; p++) {
            kip1_header_t *current_kip = (kip1_header_t *)(inis[i]->kip_data + offset);
            if (current_kip->magic != MAGIC_KIP1) {
                printf("Error: INI1s[%zu][%zu] appears not to be a KIP1!\n", i, p);
                generic_panic();
            }

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

            /* TODO: What folder should these be read out of? */
            snprintf(sd_path, sizeof(sd_path), "atmosphere/titles/%016llX/%016llX.kip", current_kip->title_id, current_kip->title_id);

            /* Try to load an override KIP from SD, if possible. */
            read_size = read_from_file(current_dst_kip, remaining_size, sd_path);
            if (read_size != 0) {
                kip1_header_t *sd_kip = (kip1_header_t *)(current_dst_kip);
                if (read_size < sizeof(kip1_header_t) || sd_kip->magic != MAGIC_KIP1) {
                    printf("Error: %s is not a KIP1?\n", sd_path);
                    generic_panic();
                } else if (sd_kip->title_id != current_kip->title_id) {
                    printf("Error: %s has wrong Title ID!\n", sd_path);
                    generic_panic();
                }
                size_t expected_sd_kip_size = kip1_get_size_from_header(sd_kip);
                if (expected_sd_kip_size != read_size) {
                    printf("Error: %s has wrong size or there is not enough space (expected 0x%zx, read 0x%zx)!\n",
                    sd_path, expected_sd_kip_size, read_size);
                    generic_panic();
                }
                remaining_size -= expected_sd_kip_size;
                current_dst_kip += expected_sd_kip_size;
            } else {
                size_t current_kip_size = kip1_get_size_from_header(current_kip);
                if (current_kip_size > remaining_size) {
                    printf("Error: Not enough space for all the KIP1s!\n");
                    generic_panic();
                }
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
