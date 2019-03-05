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
 
#ifndef FUSEE_KIP_H
#define FUSEE_KIP_H
#include "utils.h"
#include <stdint.h>

/* Fusee definitions for INI1/KIP types. This is mostly taken from hactool. */

#define MAGIC_INI1 0x31494E49
#define MAGIC_KIP1 0x3150494B
#define INI1_MAX_KIPS 0x50

typedef struct {
    uint32_t magic;
    uint32_t size;
    uint32_t num_processes;
    uint32_t _0xC;
    uint8_t kip_data[];
} ini1_header_t;

typedef struct {
    uint32_t out_offset;
    uint32_t out_size;
    uint32_t compressed_size;
    uint32_t attribute;
} kip_section_header_t;

typedef struct {
    uint32_t magic;
    char name[0xC];
    uint64_t title_id;
    uint32_t process_category;
    uint8_t main_thread_priority;
    uint8_t default_core;
    uint8_t _0x1E;
    uint8_t flags;
    kip_section_header_t section_headers[6];
    uint32_t capabilities[0x20];
    uint8_t data[];
} kip1_header_t;

static inline size_t kip1_get_size_from_header(kip1_header_t *header) {
    /* Header + .text + .rodata + .rwdata */
    return 0x100 + header->section_headers[0].compressed_size + header->section_headers[1].compressed_size + header->section_headers[2].compressed_size;
}

#endif
