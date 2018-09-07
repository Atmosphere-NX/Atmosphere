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
 
#ifndef FUSEE_LOADER_H
#define FUSEE_LOADER_H

#include "utils.h"
#include "chainloader.h"

#define LOADER_MAX_PATH_SIZE 255

typedef struct {
    char path[0x100];
    const char *key;
    uintptr_t load_address;
    size_t load_size;
} load_file_t;

typedef struct {
    const char *bct0;
    uintptr_t chainload_entrypoint;
    size_t file_id_of_entrypoint;
    size_t nb_files_to_load;
    char package2_path[LOADER_MAX_PATH_SIZE+1];
    char exosphere_path[LOADER_MAX_PATH_SIZE+1];
    char tsecfw_path[LOADER_MAX_PATH_SIZE+1];
    char warmboot_path[LOADER_MAX_PATH_SIZE+1];
    char custom_splash_path[LOADER_MAX_PATH_SIZE+1];
    char file_paths_to_load[CHAINLOADER_MAX_ENTRIES][LOADER_MAX_PATH_SIZE+1];
} loader_ctx_t;

#define LOADER_ENTRYPOINT_KEY "entrypoint"
#define LOADER_LOADLIST_KEY "loadlist"
#define LOADER_CUSTOMSPLASH_KEY "custom_splash"

#define LOADER_PACKAGE2_KEY "package2"
#define LOADER_EXOSPHERE_KEY "exosphere"
#define LOADER_TSECFW_KEY "tsecfw"
#define LOADER_WARMBOOT_KEY "warmboot"

/* TODO: Should we allow files bigger than 16 MB? */
//#define LOADER_FILESIZE_MAX 0x01000000

loader_ctx_t *get_loader_ctx(void);

void load_payload(const char *bct0);

#endif
