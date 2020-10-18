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

#include "display/video_fb.h"
#include "lib/log.h"
#include "lib/vsprintf.h"
#include "lib/ini.h"
#include "lib/fatfs/ff.h"

#include "bct0.h"

/* TODO: Is there a more concise way to do this? */
#define STAGE2_ARGV_PROGRAM_PATH 0
#define STAGE2_ARGV_ARGUMENT_STRUCT 1
#define STAGE2_ARGC 2

typedef struct {
    uint32_t version;
    ScreenLogLevel log_level;
    char bct0[BCTO_MAX_SIZE];
} stage2_args_t;

typedef struct {
    ScreenLogLevel log_level;
} stage2_mtc_args_t;

bool stage2_run_mtc(const bct0_t *bct0);
void stage2_validate_config(const bct0_t *bct0);
void stage2_load(const bct0_t *bct0);

#endif
