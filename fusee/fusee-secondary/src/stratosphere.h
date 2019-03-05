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
 
#ifndef FUSEE_STRATOSPHERE_H
#define FUSEE_STRATOSPHERE_H

#include "utils.h"
#include "kip.h"

#define STRATOSPHERE_INI1_SDFILES  0x0
#define STRATOSPHERE_INI1_EMBEDDED 0x1
#define STRATOSPHERE_INI1_PACKAGE2 0x2
#define STRATOSPHERE_INI1_MAX      0x3

ini1_header_t *stratosphere_get_ini1(uint32_t target_firmware);
ini1_header_t *stratosphere_get_sd_files_ini1(void);
void stratosphere_free_ini1(void);

ini1_header_t *stratosphere_merge_inis(ini1_header_t **inis, unsigned int num_inis);

typedef struct {
    bool has_nogc_config;
    bool enable_nogc;
} stratosphere_cfg_t;

#define STRATOSPHERE_NOGC_KEY "nogc"

#endif
