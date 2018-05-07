#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include "utils.h"
#include "loader.h"
#include "sd_utils.h"
#include "stage2.h"
#include "lib/ini.h"

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
                strncpy(load_file_ctx->path, value, sizeof(load_file_ctx->path));
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

bool validate_load_address(uintptr_t load_addr) {
    /* TODO: Actually validate addresses. */
    (void)(load_addr);
    return true;
}

void load_list_entry(const char *key) {
    load_file_t load_file_ctx = {0};
    struct stat st;
    size_t size;
    size_t entry_num;
    chainloader_entry_t *entry;
    load_file_ctx.key = key;

    if (ini_parse_string(get_loader_ctx()->bct0, loadlist_entry_ini_handler, &load_file_ctx) < 0) {
        printf("Error: Failed to parse BCT.ini!\n");
        generic_panic();
    }

    if (load_file_ctx.load_address == 0 || load_file_ctx.path[0] == '\x00') {
        printf("Error: Failed to determine where to load %s!\n", key);
        generic_panic();
    }

    if (!check_32bit_address_loadable(load_file_ctx.load_address)) {
        printf("Error: Load address 0x%08x is invalid (%s)!\n", load_file_ctx.load_address, key);
        generic_panic();
    }

    if (stat(load_file_ctx.path, &st) == -1) {
        printf("Error: Failed to stat file %s: %s!\n", load_file_ctx.path, strerror(errno));
        generic_panic();
    }

    size = (size_t)st.st_size;
    if (!check_32bit_address_range_loadable(load_file_ctx.load_address, size)) {
        printf("Error: %s has an invalid load address & size combination!\n", key);
        generic_panic();
    }

    entry_num = g_chainloader_num_entries++;
    entry = &g_chainloader_entries[entry_num];
    entry->load_address = load_file_ctx.load_address;
    entry->size = size;
    entry->num = entry_num;
    strcpy(get_loader_ctx()->file_paths[entry_num], load_file_ctx.path);
    get_loader_ctx()->nb_files++;

    /* Check for special keys. */
    if (strcmp(key, LOADER_PACKAGE2_KEY) == 0) {
        get_loader_ctx()->package2_loadfile = load_file_ctx;
    } else if (strcmp(key, LOADER_EXOSPHERE_KEY) == 0) {
        get_loader_ctx()->exosphere_loadfile = load_file_ctx;
    } else if (strcmp(key, LOADER_TSECFW_KEY) == 0) {
        get_loader_ctx()->tsecfw_loadfile = load_file_ctx;
    } else if (strcmp(key, LOADER_WARMBOOT_KEY) == 0) {
        get_loader_ctx()->warmboot_loadfile = load_file_ctx;
    }
}

void parse_loadlist(const char *ll) {
    printf("Parsing load list: %s\n", ll);

    char load_list[0x200] = {0};
    strncpy(load_list, ll, 0x200);

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
                printf("Error: Load list is malformed!\n");
                generic_panic();
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
            strncpy(loader_ctx->custom_splash_path, value, sizeof(loader_ctx->custom_splash_path));
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
        printf("Error: Failed to parse BCT.ini!\n");
        generic_panic();
    }

    /* Sort the entries by ascending load addresses */
    qsort(g_chainloader_entries, g_chainloader_num_entries, sizeof(chainloader_entry_t), chainloader_entry_comparator);

    /* Check for possible overlap and find the entrypoint, also determine whether we can load in-place */
    ctx->file_id_of_entrypoint = ctx->nb_files;
    for (size_t i = 0; i < ctx->nb_files; i++) {
        if (i != ctx->nb_files - 1 &&
            overlaps(
                g_chainloader_entries[i].load_address,
                g_chainloader_entries[i].load_address + g_chainloader_entries[i].size,
                g_chainloader_entries[i + 1].load_address,
                g_chainloader_entries[i + 1].load_address + g_chainloader_entries[i + 1].size
            )
        ) {
            printf(
                "Error: Loading ranges for files %s and %s overlap!\n",
                ctx->file_paths[g_chainloader_entries[i].num],
                ctx->file_paths[g_chainloader_entries[i + 1].num]
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

    if (ctx->chainload_entrypoint != 0 && ctx->file_id_of_entrypoint >= ctx->nb_files) {
        printf("Error: Entrypoint not in range of any of the files!\n");
        generic_panic();
    }

    if (ctx->chainload_entrypoint == 0 && !can_load_in_place) {
        printf("Error: Files have to be loaded in place when booting Horizon!\n");
        generic_panic();
    }

    if (can_load_in_place) {
        for (size_t i = 0; i < ctx->nb_files; i++) {
            chainloader_entry_t *entry = &g_chainloader_entries[i];
            entry->src_address = entry->load_address;
            if (read_sd_file((void *)entry->src_address, entry->size, ctx->file_paths[entry->num]) != entry->size) {
                printf("Error: Failed to read file %s: %s!\n", ctx->file_paths[entry->num], strerror(errno));
                generic_panic();
            }
        }
    } else {
        size_t total_size = 0;
        uintptr_t carveout, min_addr, max_addr, pos;

        for (size_t i = 0; i < ctx->nb_files; i++) {
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
            printf("Error: Failed to find a carveout!\n");
            generic_panic();
        }

        /* Finally, read the files into the carveout */
        pos = carveout;
        for (size_t i = 0; i < ctx->nb_files; i++) {
            chainloader_entry_t *entry = &g_chainloader_entries[i];
            entry->src_address = pos;
            if (read_sd_file((void *)entry->src_address, entry->size, ctx->file_paths[entry->num]) != entry->size) {
                printf("Error: Failed to read file %s: %s!\n", ctx->file_paths[entry->num], strerror(errno));
                generic_panic();
            }
            pos += entry->size;
        }
    }
}
