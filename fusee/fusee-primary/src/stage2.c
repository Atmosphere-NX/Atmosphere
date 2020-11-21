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

#include "stage2.h"
#include "chainloader.h"
#include "fs_utils.h"
#include "utils.h"

bool stage2_run_mtc(const bct0_t *bct0) {
    static bool has_run_mtc = false;
    FILINFO info;
    size_t size;

    if (has_run_mtc) {
        return true;
    }
    
    /* Check if the MTC binary is present. */
    if (f_stat(bct0->stage2_mtc_path, &info) != FR_OK) {
        print(SCREEN_LOG_LEVEL_WARNING, "Stage2's MTC binary not found!\n");
        return false;
    }
    
    size = (size_t)info.fsize;
    
    /* Try to read the MTC binary. */
    if (read_from_file((void *)bct0->stage2_load_address, size, bct0->stage2_mtc_path) != size) {
        print(SCREEN_LOG_LEVEL_WARNING, "Failed to read stage2's MTC binary (%s)!\n", bct0->stage2_mtc_path);
        return false;
    }
    
    ScreenLogLevel mtc_log_level = log_get_log_level();
    bool mtc_res = false;
    int mtc_argc = 1;
    char mtc_arg_data[CHAINLOADER_ARG_DATA_MAX_SIZE] = {0};
    stage2_mtc_args_t *mtc_args = (stage2_mtc_args_t *)mtc_arg_data;

    /* Setup argument data. */
    memcpy(&mtc_args->log_level, &mtc_log_level, sizeof(mtc_log_level));
    
    /* Run the MTC binary. */
    mtc_res = (((int (*)(int, void *))bct0->stage2_load_address)(mtc_argc, mtc_arg_data) == 0);
    
    /* Cleanup right away. */
    memset((void *)bct0->stage2_load_address, 0, size);

    has_run_mtc = true;
    
    return mtc_res;
}

void stage2_validate_config(const bct0_t *bct0) {
    if (bct0->stage2_load_address == 0 || bct0->stage2_path[0] == '\x00') {
        fatal_error("Failed to determine where to load stage2!\n");
    }

    if (strlen(bct0->stage2_path) + 1 + sizeof(stage2_args_t) > CHAINLOADER_ARG_DATA_MAX_SIZE) {
        fatal_error("Stage2's path name is too big!\n");
    }

    if (!check_32bit_address_loadable(bct0->stage2_entrypoint)) {
        fatal_error("Stage2's entrypoint is invalid!\n");
    }

    if (!check_32bit_address_loadable(bct0->stage2_load_address)) {
        fatal_error("Stage2's load address is invalid!\n");
    }
    
    print(SCREEN_LOG_LEVEL_DEBUG, "Stage 2 Config:\n");
    print(SCREEN_LOG_LEVEL_DEBUG | SCREEN_LOG_LEVEL_NO_PREFIX, "    File Path:    %s\n", bct0->stage2_path);
    print(SCREEN_LOG_LEVEL_DEBUG | SCREEN_LOG_LEVEL_NO_PREFIX, "    MTC File Path:    %s\n", bct0->stage2_mtc_path);
    print(SCREEN_LOG_LEVEL_DEBUG | SCREEN_LOG_LEVEL_NO_PREFIX, "    Load Address: 0x%08x\n", bct0->stage2_load_address);
    print(SCREEN_LOG_LEVEL_DEBUG | SCREEN_LOG_LEVEL_NO_PREFIX, "    Entrypoint:   0x%p\n", bct0->stage2_entrypoint);
}

void stage2_load(const bct0_t *bct0) {
    FILINFO info;
    size_t size;
    uintptr_t tmp_addr;

    if (f_stat(bct0->stage2_path, &info) != FR_OK) {
        fatal_error("Failed to stat stage2 (%s)!\n", bct0->stage2_path);
    }

    size = (size_t)info.fsize;

    /* the LFB is located at 0xC0000000 atm */
    if (size > 0xC0000000u - 0x80000000u) {
        fatal_error("Stage2 is way too big!\n");
    }

    if (!check_32bit_address_range_loadable(bct0->stage2_load_address, size)) {
        fatal_error("Stage2 has an invalid load address & size combination (0x%08x 0x%08x)!\n", bct0->stage2_load_address, size);
    }

    if (bct0->stage2_entrypoint < bct0->stage2_load_address || bct0->stage2_entrypoint >= bct0->stage2_load_address + size) {
        fatal_error("Stage2's entrypoint is outside Stage2!\n");
    }

    if (check_32bit_address_range_in_program(bct0->stage2_load_address, size)) {
        tmp_addr = 0x80000000u;
    } else {
        tmp_addr = bct0->stage2_load_address;
    }
    
    /* Try to read stage2. */
    if (read_from_file((void *)tmp_addr, size, bct0->stage2_path) != size) {
        fatal_error("Failed to read stage2 (%s)!\n", bct0->stage2_path);
    }

    g_chainloader_num_entries             = 1;
    g_chainloader_entries[0].load_address = bct0->stage2_load_address;
    g_chainloader_entries[0].src_address  = tmp_addr;
    g_chainloader_entries[0].size         = size;
    g_chainloader_entries[0].num          = 0;
    g_chainloader_entrypoint              = bct0->stage2_entrypoint;
}
