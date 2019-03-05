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

#include "stage2.h"
#include "chainloader.h"
#include "fs_utils.h"
#include "utils.h"

char g_stage2_path[0x100] = {0};

const char *stage2_get_program_path(void) {
    return g_stage2_path;
}

/* We get the luxury of assuming a constant filename/load address. */
void load_stage2(void) {
    FILINFO info;
    size_t size;
    uintptr_t tmp_addr;
    stage2_config_t config = {
        .path = "sept/payload.bin",
        .load_address = 0xF0000000,
        .entrypoint = 0xF0000000,
    };
    
    print(SCREEN_LOG_LEVEL_DEBUG, "Stage 2 Config:\n");
    print(SCREEN_LOG_LEVEL_DEBUG | SCREEN_LOG_LEVEL_NO_PREFIX, "    File Path:    %s\n", config.path);
    print(SCREEN_LOG_LEVEL_DEBUG | SCREEN_LOG_LEVEL_NO_PREFIX, "    Load Address: 0x%08x\n", config.load_address);
    print(SCREEN_LOG_LEVEL_DEBUG | SCREEN_LOG_LEVEL_NO_PREFIX, "    Entrypoint:   0x%p\n", config.entrypoint);

    if (f_stat(config.path, &info) != FR_OK) {
        fatal_error("Failed to stat stage2 (%s)!\n", config.path);
    }

    size = (size_t)info.fsize;

    /* the LFB is located at 0xC0000000 atm */
    if (size > 0xC0000000u - 0x80000000u) {
        fatal_error("Stage2 is way too big!\n");
    }

    if (!check_32bit_address_range_loadable(config.load_address, size)) {
        fatal_error("Stage2 has an invalid load address & size combination (0x%08x 0x%08x)!\n", config.load_address, size);
    }

    if (config.entrypoint < config.load_address || config.entrypoint >= config.load_address + size) {
        fatal_error("Stage2's entrypoint is outside Stage2!\n");
    }

    if (check_32bit_address_range_in_program(config.load_address, size)) {
        tmp_addr = 0x80000000u;
    } else {
        tmp_addr = config.load_address;
    }
    
    if (read_from_file((void *)tmp_addr, size, config.path) != size) {
        fatal_error("Failed to read stage2 (%s)!\n", config.path);
    }

    g_chainloader_num_entries             = 1;
    g_chainloader_entries[0].load_address = config.load_address;
    g_chainloader_entries[0].src_address  = tmp_addr;
    g_chainloader_entries[0].size         = size;
    g_chainloader_entries[0].num          = 0;
    g_chainloader_entrypoint              = config.entrypoint;

    strncpy(g_stage2_path, config.path, sizeof(g_stage2_path) - 1);
    g_stage2_path[sizeof(g_stage2_path) - 1]  = '\0';
}
