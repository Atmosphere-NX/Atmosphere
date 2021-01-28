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
 
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include "utils.h"
#include "loader.h"
#include "fs_utils.h"
#include "stage2.h"
#include "../../../fusee/common/ini.h"
#include "../../../fusee/common/log.h"

static loader_ctx_t g_loader_ctx = {0};

loader_ctx_t *get_loader_ctx(void) {
    return &g_loader_ctx;
}

static int loadlist_entry_ini_handler(void *user, const char *section, const char *name, const char *value) {
    load_file_t *load_file_ctx = (load_file_t *)user;
    uintptr_t x = 0;
    const char *ext = NULL;

    if (strcmp(section, "stage2") == 0) {
        if (strstr(name, load_file_ctx->key) == name) {
            ext = name + strlen(load_file_ctx->key);
            if (strcmp(ext, "_path") == 0) {
                /* Copy in the path. */
                strncpy(load_file_ctx->path, value, sizeof(load_file_ctx->path) - 1);
                load_file_ctx->path[sizeof(load_file_ctx->path) - 1] = '\0';
            } else if (strcmp(ext, "_addr") == 0) {
                /* Read in load address as a hex string. */
                sscanf(value, "%x", &x);
                load_file_ctx->load_address = x;
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    } else {
        return 0;
    }
    return 1;
}

static void load_list_entry(const char *key) {
    load_file_t load_file_ctx = {0};
    struct stat st;
    size_t size;
    size_t entry_num;
    chainloader_entry_t *entry;
    load_file_ctx.key = key;

    if (ini_parse_string(get_loader_ctx()->bct0, loadlist_entry_ini_handler, &load_file_ctx) < 0) {
        fatal_error("Failed to parse BCT.ini!\n");
    }

    if (load_file_ctx.load_address == 0 || load_file_ctx.path[0] == '\x00') {
        fatal_error("Failed to determine where to load %s from!\n", key);
    }

    if (strlen(load_file_ctx.path) > LOADER_MAX_PATH_SIZE) {
        fatal_error("The filename for %s is too long!\n", key);
    }

    if (!check_32bit_address_loadable(load_file_ctx.load_address)) {
        fatal_error("Load address 0x%08x is invalid (%s)!\n", load_file_ctx.load_address, key);
    }

    if (stat(load_file_ctx.path, &st) == -1) {
        fatal_error("Failed to stat file %s: %s!\n", load_file_ctx.path, strerror(errno));
    }

    size = (size_t)st.st_size;
    if (!check_32bit_address_range_loadable(load_file_ctx.load_address, size)) {
        fatal_error("%s has an invalid load address & size combination!\n", key);
    }

    entry_num = g_chainloader_num_entries++;
    entry = &g_chainloader_entries[entry_num];
    entry->load_address = load_file_ctx.load_address;
    entry->size = size;
    entry->num = entry_num;
    get_loader_ctx()->nb_files_to_load++;

    strcpy(get_loader_ctx()->file_paths_to_load[entry_num], load_file_ctx.path);
}

static void parse_loadlist(const char *ll) {
    print(SCREEN_LOG_LEVEL_DEBUG, "Parsing load list: %s\n", ll);

    char load_list[0x200] = {0};
    strncpy(load_list, ll, 0x200 - 1);

    char *entry, *p;

    /* entry will point to the start of the current entry. p will point to the current location in the list. */
    entry = load_list;
    p = load_list;

    while (*p) {
        if (*p == ' ' || *p == '\t') {
            /* We're at the end of an entry. */
            *p = '\x00';

            /* Load the entry. */
            load_list_entry(entry);
            /* Skip to the next delimiter. */
            for (; *p == ' ' || *p == '\t' || *p == '\x00'; p++) { }
            if (*p != '|') {
                fatal_error("Load list is malformed!\n");
            } else {
                /* Skip to the next entry. */
                for (; *p == ' ' || *p == '\t' || *p == '|'; p++) { }
                entry = p;
            }
        }

        p++;
        if (*p == '\x00') {
            /* We're at the end of the line, load the last entry */
            load_list_entry(entry);
        }
    }
}

static int loadlist_ini_handler(void *user, const char *section, const char *name, const char *value) {
    loader_ctx_t *loader_ctx = (loader_ctx_t *)user;
    uintptr_t x = 0;

    if (strcmp(section, "stage2") == 0) {
        if (strcmp(name, LOADER_LOADLIST_KEY) == 0) {
            parse_loadlist(value);
        } else if (strcmp(name, LOADER_ENTRYPOINT_KEY) == 0) {
            /* Read in entrypoint as a hex string. */
            sscanf(value, "%x", &x);
            loader_ctx->chainload_entrypoint = x;
        } else if (strcmp(name, LOADER_CUSTOMSPLASH_KEY) == 0) {
            strncpy(loader_ctx->custom_splash_path, value, LOADER_MAX_PATH_SIZE);
            loader_ctx->custom_splash_path[LOADER_MAX_PATH_SIZE] = '\0';
        } else if (strcmp(name, LOADER_PACKAGE2_KEY) == 0) {
            strncpy(loader_ctx->package2_path, value, LOADER_MAX_PATH_SIZE);
            loader_ctx->package2_path[LOADER_MAX_PATH_SIZE] = '\0';
        } else if (strcmp(name, LOADER_EXOSPHERE_KEY) == 0) {
            strncpy(loader_ctx->exosphere_path, value, LOADER_MAX_PATH_SIZE);
            loader_ctx->exosphere_path[LOADER_MAX_PATH_SIZE] = '\0';
        } else if (strcmp(name, LOADER_TSECFW_KEY) == 0) {
            strncpy(loader_ctx->tsecfw_path, value, LOADER_MAX_PATH_SIZE);
            loader_ctx->tsecfw_path[LOADER_MAX_PATH_SIZE] = '\0';
        } else if (strcmp(name, LOADER_WARMBOOT_KEY) == 0) {
            strncpy(loader_ctx->warmboot_path, value, LOADER_MAX_PATH_SIZE);
            loader_ctx->warmboot_path[LOADER_MAX_PATH_SIZE] = '\0';
        } else {
            return 0;
        }
    } else {
        return 0;
    }
    return 1;
}

static int chainloader_entry_comparator(const void *a, const void *b) {
    const chainloader_entry_t *e1 = (const chainloader_entry_t *)a;
    const chainloader_entry_t *e2 = (const chainloader_entry_t *)b;

    return (int)(e1->load_address - e2->load_address);
}

static uintptr_t find_carveout(chainloader_entry_t *entries, size_t num_entries, size_t requested) {
    for(size_t i = 0; i < num_entries; i++) {
        uintptr_t lbound = entries[i].load_address + entries[i].size;
        if (check_32bit_address_range_loadable(lbound, requested)) {
            if (i == num_entries - 1 || lbound + requested <= entries[i + 1].load_address)
                return lbound;
        }
    }

    return 0;
}

void load_payload(const char *bct0) {
    loader_ctx_t *ctx = get_loader_ctx();
    bool can_load_in_place = true;

    extern uint8_t __chainloader_start__[], __chainloader_end__[];
    extern uint8_t __stack_bottom__[], __stack_top__[];
    extern uint8_t __heap_start__[], __heap_end__[];
    extern uint8_t __start__[], __end__[];

    static chainloader_entry_t nonallocatable_entries[] = {
        {.load_address = 0x00000000, .size = 0x40010000}, /* Unknown/Invalid + Low IRAM */
        {.load_address = 0x40040000, .size = 0x80000000 - 0x40040000}, /* Invalid/MMIO */
        {.load_address = (uintptr_t)__chainloader_start__, .size = 0},
        {.load_address = (uintptr_t)__stack_bottom__, .size = 0},
        {.load_address = (uintptr_t)__heap_start__, .size = 0},
        {.load_address = (uintptr_t)__start__, .size = 0},
        {.load_address = 0, .size = 0}, /* Min to max loaded address */
    };
    static const size_t num_nonallocatable_entries = sizeof(nonallocatable_entries) / sizeof(nonallocatable_entries[0]);
    if (nonallocatable_entries[2].size == 0) {
        /* Initializers aren't computable at load time */
        nonallocatable_entries[2].size = __chainloader_end__ - __chainloader_start__;
        nonallocatable_entries[3].size = __stack_top__ - __stack_bottom__;
        nonallocatable_entries[4].size = __heap_end__ - __heap_start__;
        nonallocatable_entries[5].size = __end__ - __start__;
    }

    /* Set BCT0 global. */
    ctx->bct0 = bct0;

    if (ini_parse_string(ctx->bct0, loadlist_ini_handler, ctx) < 0) {
        fatal_error("Failed to parse BCT.ini!\n");
    }

    if (ctx->chainload_entrypoint != 0 || ctx->nb_files_to_load > 0) {
        fatal_error("loadlist must be empty when booting Horizon!\n");
    }

    /* Sort the entries by ascending load addresses */
    qsort(g_chainloader_entries, g_chainloader_num_entries, sizeof(chainloader_entry_t), chainloader_entry_comparator);

    /* Check for possible overlap and find the entrypoint, also determine whether we can load in-place */
    ctx->file_id_of_entrypoint = ctx->nb_files_to_load;
    for (size_t i = 0; i < ctx->nb_files_to_load; i++) {
        if (i != ctx->nb_files_to_load - 1 &&
            overlaps(
                g_chainloader_entries[i].load_address,
                g_chainloader_entries[i].load_address + g_chainloader_entries[i].size,
                g_chainloader_entries[i + 1].load_address,
                g_chainloader_entries[i + 1].load_address + g_chainloader_entries[i + 1].size
            )
        ) {
            fatal_error(
                "Loading ranges for files %s and %s overlap!\n",
                ctx->file_paths_to_load[g_chainloader_entries[i].num],
                ctx->file_paths_to_load[g_chainloader_entries[i + 1].num]
            );
            generic_panic();
        }

        if(ctx->chainload_entrypoint >= g_chainloader_entries[i].load_address &&
        ctx->chainload_entrypoint < g_chainloader_entries[i].load_address + g_chainloader_entries[i].size) {
            ctx->file_id_of_entrypoint = g_chainloader_entries[i].num;
        }

        can_load_in_place = can_load_in_place &&
        check_32bit_address_range_in_program(
            g_chainloader_entries[i].load_address,
            g_chainloader_entries[i].load_address + g_chainloader_entries[i].size
        );
    }

    if (ctx->chainload_entrypoint != 0 && ctx->file_id_of_entrypoint >= ctx->nb_files_to_load) {
        fatal_error("Entrypoint not in range of any of the files!\n");
    }

    if (can_load_in_place) {
        for (size_t i = 0; i < ctx->nb_files_to_load; i++) {
            chainloader_entry_t *entry = &g_chainloader_entries[i];
            entry->src_address = entry->load_address;
            if (read_from_file((void *)entry->src_address, entry->size, ctx->file_paths_to_load[entry->num]) != entry->size) {
                fatal_error("Failed to read file %s: %s!\n", ctx->file_paths_to_load[entry->num], strerror(errno));
            }
        }
    } else {
        size_t total_size = 0;
        uintptr_t carveout, min_addr, max_addr, pos;

        for (size_t i = 0; i < ctx->nb_files_to_load; i++) {
            total_size += g_chainloader_entries[i].size;
        }

        min_addr = g_chainloader_entries[g_chainloader_num_entries - 1].load_address;
        max_addr = g_chainloader_entries[g_chainloader_num_entries - 1].load_address +
        g_chainloader_entries[g_chainloader_num_entries - 1].size;

        nonallocatable_entries[num_nonallocatable_entries - 1].load_address = min_addr;
        nonallocatable_entries[num_nonallocatable_entries - 1].size = max_addr - min_addr;

        /* We need to find a carveout, first outside the loaded range, then inside */
        qsort(nonallocatable_entries, num_nonallocatable_entries, sizeof(chainloader_entry_t), chainloader_entry_comparator);

        carveout = find_carveout(nonallocatable_entries, num_nonallocatable_entries, total_size);
        if (carveout == 0) {
            carveout = find_carveout(g_chainloader_entries, g_chainloader_num_entries, total_size);
        }
        if (carveout == 0) {
            fatal_error("Failed to find a carveout!\n");
        }

        /* Finally, read the files into the carveout */
        pos = carveout;
        for (size_t i = 0; i < ctx->nb_files_to_load; i++) {
            chainloader_entry_t *entry = &g_chainloader_entries[i];
            entry->src_address = pos;
            if (read_from_file((void *)entry->src_address, entry->size, ctx->file_paths_to_load[entry->num]) != entry->size) {
                fatal_error("Failed to read file %s: %s!\n", ctx->file_paths_to_load[entry->num], strerror(errno));
            }
            pos += entry->size;
        }
    }
}
