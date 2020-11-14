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
 
#ifndef FUSEE_GPT_H
#define FUSEE_GPT_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct efi_entry_t {
    uint8_t type_uuid[16];
    uint8_t unique_uuid[16];
    uint64_t first_lba;
    uint64_t last_lba; /* Inclusive */
    uint64_t attributes;
    uint16_t name[36];
} efi_entry_t;

typedef struct efi_header {
    uint8_t magic[8]; /* Must be "EFI PART" */
    uint32_t revision;

    uint32_t header_size; /* Usually and at most 92 bytes. */
    uint32_t header_crc32; /* 0 during the computation */
    uint32_t reserved; /* Must be 0. */

    uint64_t header_lba;
    uint64_t backup_lba;
    uint64_t partitions_first_lba;
    uint64_t partitions_last_lba;

    uint8_t disk_guid[16];

    uint64_t entries_first_lba;
    uint32_t entry_count;
    uint32_t entry_size;
    uint32_t entries_crc32;
} __attribute__((packed, aligned(4))) efi_header_t;

typedef int (*gpt_entry_iterator_t)(const efi_entry_t *entry, void *param, size_t entry_offset, FILE *disk);
typedef int (*gpt_emu_entry_iterator_t)(const efi_entry_t *entry, void *param, size_t entry_offset, FILE *disk, const char *origin_path, int num_parts, uint64_t part_limit);

int gpt_get_header(efi_header_t *out, FILE *disk, size_t sector_size);
int gpt_iterate_through_entries(FILE *disk, size_t sector_size, gpt_entry_iterator_t callback, void *param);

#endif
