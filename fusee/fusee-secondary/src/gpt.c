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
 
#include <string.h>
#include <errno.h>
#include "gpt.h"

int gpt_get_header(efi_header_t *out, FILE *disk, size_t sector_size) {
    union {
        uint8_t sector[512];
        efi_header_t hdr;
    } d;

    if (disk == NULL || out == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Read and check the protective MBR. */
    if (fread(d.sector, 512, 1, disk) == 0) {
        return -1;
    }
    if (fseek(disk, sector_size - 512, SEEK_CUR) != 0) {
        return -1;
    }

    if (d.sector[0x1FE] != 0x55 || d.sector[0x1FF] != 0xAA) {
        errno = EILSEQ;
        return -1;
    }

    /* We only allow for the GPT header to be at sector 1 (like nx-bootloader). */
    if (fread(d.sector, 512, 1, disk) == 0) {
        return -1;
    }
    if (fseek(disk, sector_size - 512, SEEK_CUR) != 0) {
        return -1;
    }
    /* Check for "EFI PART". */
    if (memcmp(d.hdr.magic, "EFI PART", 8) != 0) {
        errno = EILSEQ;
        return -1;
    }
    /* Check whether we're really at sector 1, etc. (we won't check backup_lba). */
    if (d.hdr.header_lba != 1 || d.hdr.entries_first_lba < 2 || d.hdr.partitions_last_lba < d.hdr.partitions_first_lba) {
        errno = EILSEQ;
        return -1;
    }
    if (d.hdr.header_size < sizeof(efi_header_t) || d.hdr.entry_size < sizeof(efi_entry_t)) {
        errno = EILSEQ;
        return -1;
    }
    /* Some more checks: */
    if(d.hdr.header_size > sector_size || d.hdr.revision != 0x10000) {
        errno = ENOTSUP;
        return -1;
    }

    memcpy(out, &d.hdr, sizeof(efi_header_t));

    return 0;
}

int gpt_iterate_through_entries(FILE *disk, size_t sector_size, gpt_entry_iterator_t callback, void *param) {
    efi_header_t hdr;
    efi_entry_t entry;
    size_t offset = 2 * 512; /* Sector #2. */
    size_t delta;

    /* Get the header. */
    if (gpt_get_header(&hdr, disk, sector_size) == -1) {
        return -1;
    }

    /* Seek to the entry table. */
    if (fseek(disk, sector_size * hdr.entries_first_lba - offset, SEEK_CUR) != 0) {
        return -1;
    }

    offset = sector_size * hdr.entries_first_lba;
    delta = hdr.entry_size - sizeof(efi_entry_t);

    /* Iterate through the entries. */
    for (uint32_t i = 0; i < hdr.entry_count; i++) {
        if (!fread(&entry, sizeof(efi_entry_t), 1, disk)) {
            return -1;
        }

        if (callback(&entry, param, offset, disk) != 0) {
            return -1;
        }

        if (delta != 0 && fseek(disk, delta, SEEK_CUR) != 0) {
            return -1;
        }

        offset += hdr.entry_size;
    }

    return 0;
}
