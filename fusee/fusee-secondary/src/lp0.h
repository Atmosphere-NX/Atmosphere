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
 
#ifndef FUSEE_WARMBOOT_BIN_LP0_H
#define FUSEE_WARMBOOT_BIN_LP0_H

#include "utils.h"

/* WBT0 */
#define WARMBOOT_MAGIC 0x30544257

typedef struct {
    uint32_t entrypoint_insn;
    uint32_t magic;
    uint32_t target_firmware;
    uint32_t padding[2];
} warmboot_metadata_t;

typedef struct {
    uint32_t warmboot_fw_size;
    uint32_t dst_address;
    uint32_t entrypoint;
    uint32_t code_size;
} warmboot_nv_header_t;

typedef struct {
    uint32_t warmboot_fw_size;
    uint32_t _0x4[3];
    uint8_t  rsa_modulus[0x100];
    uint8_t  _0x110[0x10];
    uint8_t  rsa_signature[0x100];
    uint8_t  _0x220[0x10];
    warmboot_nv_header_t nv_header;
    warmboot_metadata_t ams_metadata;
} warmboot_ams_header_t;

_Static_assert(sizeof(warmboot_ams_header_t) == 0x254, "Warmboot header definition!");

#endif
