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
 
#ifndef FUSEE_STAGE2_H
#define FUSEE_STAGE2_H

#include <stdbool.h>
#include <stdint.h>

#include "../../../fusee/common/display/video_fb.h"
#include "../../../fusee/common/log.h"
#include "../../../fusee/common/vsprintf.h"
#include "../../../fusee/common/ini.h"
#include "../../../fusee/common/fatfs/ff.h"

/* TODO: Is there a more concise way to do this? */
#define STAGE2_ARGV_PROGRAM_PATH 0
#define STAGE2_ARGV_ARGUMENT_STRUCT 1
#define STAGE2_ARGC 2

#define STAGE2_NAME_KEY "stage2_path"
#define STAGE2_MTC_NAME_KEY "stage2_mtc_path"
#define STAGE2_ADDRESS_KEY "stage2_addr"
#define STAGE2_ENTRYPOINT_KEY "stage2_entrypoint"

#define BCTO_MAX_SIZE 0x5800

typedef struct {
    char path[0x100];
    char mtc_path[0x100];
    uintptr_t load_address;
    uintptr_t entrypoint;
} stage2_config_t;

typedef struct {
    uint32_t version;
    ScreenLogLevel log_level;
    char bct0[BCTO_MAX_SIZE];
} stage2_args_t;

typedef struct {
    ScreenLogLevel log_level;
} stage2_mtc_args_t;

const char *stage2_get_program_path(void);
void load_stage2(void);

#endif