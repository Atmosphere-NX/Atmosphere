#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "utils.h"
#include "package2.h"
#include "stratosphere.h"
#include "sd_utils.h"

static uint8_t *g_stratosphere_ini1;
static bool g_initialized_stratosphere_ini1 = false;

static uint8_t *g_ini1_buffer;

void stratosphere_allocate_mem(void) {
    /* TODO call it*/
    g_stratosphere_ini1 = (uint8_t *)malloc(PACKAGE2_SIZE_MAX);
    g_ini1_buffer = (uint8_t *)malloc(PACKAGE2_SIZE_MAX);
}

void stratosphere_free_mem(void) {
    /* TODO call it*/
    free(g_stratosphere_ini1);
    free(g_ini1_buffer);
}

ini1_header_t *stratosphere_get_ini1(void) {
    ini1_header_t *ini1_header = (ini1_header_t *)g_stratosphere_ini1;
    if (g_initialized_stratosphere_ini1) {
        return ini1_header;
    }
    ini1_header->magic = MAGIC_INI1;
    ini1_header->size = sizeof(ini1_header_t);
    ini1_header->num_processes = 0;
    ini1_header->_0xC = 0;

    /* TODO: When we have processes, copy them into ini1_header->kip_data here. */

    g_initialized_stratosphere_ini1 = true;
    return ini1_header;
}

/* Merges some number of INI1s into a single INI1. It's assumed that the INIs are in order of preference. */
size_t stratosphere_merge_inis(void *dst, ini1_header_t **inis, unsigned int num_inis) {
    char sd_path[0x100] = {0};
    /* Validate all ini headers. */
    for (unsigned int i = 0; i < num_inis; i++) {
        if (inis[i] == NULL || inis[i]->magic != MAGIC_INI1 || inis[i]->num_processes > INI1_MAX_KIPS) {
            printf("Error: INI1s[%d] section appears to not contain an INI1!\n", i);
            generic_panic();
        }
    }

    uint64_t process_list[INI1_MAX_KIPS] = {0};

    memset(g_ini1_buffer, 0, PACKAGE2_SIZE_MAX);
    ini1_header_t *merged = (ini1_header_t *)g_ini1_buffer;
    merged->magic = MAGIC_INI1;
    merged->num_processes = 0;
    merged->_0xC = 0;
    size_t remaining_size = PACKAGE2_SIZE_MAX - sizeof(ini1_header_t);

    unsigned char *current_dst_kip = merged->kip_data;

    /* Actually merge into the inis. */
    for (unsigned int i = 0; i < num_inis; i++) {
        uint64_t offset = 0;
        for (unsigned int p = 0; p < inis[i]->num_processes; p++) {
            kip1_header_t *current_kip = (kip1_header_t *)(inis[i]->kip_data + offset);
            if (current_kip->magic != MAGIC_KIP1) {
                printf("Error: INI1s[%d][%d] appears not to be a KIP1!\n", i, p);
                generic_panic();
            }


            bool already_loaded = false;
            for (unsigned int j = 0; j < merged->num_processes; j++) {
                if (process_list[j] == current_kip->title_id) {
                    already_loaded = true;
                    break;
                }
            }
            if (already_loaded) {
                continue;
            }

            /* TODO: What folder should these be read out of? */
            snprintf(sd_path, sizeof(sd_path), "atmosphere/titles/%016llx/%016llx.kip", current_kip->title_id, current_kip->title_id);

            /* Try to load an override KIP from SD, if possible. */
            if (read_sd_file(current_dst_kip, remaining_size, sd_path)) {
                kip1_header_t *sd_kip = (kip1_header_t *)(current_dst_kip);
                if (sd_kip->magic != MAGIC_KIP1) {
                    printf("Error: %s is not a KIP1?\n", sd_path);
                    generic_panic();
                } else if (sd_kip->title_id != current_kip->title_id) {
                    printf("Error: %s has wrong Title ID!\n", sd_path);
                    generic_panic();
                }
                uint64_t sd_kip_size = kip1_get_size_from_header(sd_kip);
                remaining_size -= sd_kip_size;
                current_dst_kip += sd_kip_size;
            } else {
                uint64_t current_kip_size = kip1_get_size_from_header(current_kip);
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

    /* Copy merged INI1 to destination. */
    memcpy(dst, merged, merged->size);

    return merged->size;
}
