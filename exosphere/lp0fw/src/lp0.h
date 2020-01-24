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
 
#ifndef EXOSPHERE_WARMBOOT_BIN_LP0_H
#define EXOSPHERE_WARMBOOT_BIN_LP0_H

#include "utils.h"

/* WBT0 */
#define WARMBOOT_MAGIC 0x30544257

typedef struct {
    uint32_t magic;
    uint32_t target_firmware;
    uint32_t padding[2];
} warmboot_metadata_t;

void lp0_entry_main(warmboot_metadata_t *meta);

void __attribute__((noreturn)) reboot(void);

#endif
