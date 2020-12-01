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

#ifndef FUSEE_PACKAGE2_H
#define FUSEE_PACKAGE2_H

/* This is a library for patching Package2 prior to handoff to Exosphere. */

#define MAGIC_PK21 (0x31324B50)
#define PACKAGE2_SIZE_MAX 0x7FC000

#define PACKAGE2_SECTION_KERNEL 0x0
#define PACKAGE2_SECTION_INI1   0x1
#define PACKAGE2_SECTION_UNUSED 0x2
#define PACKAGE2_SECTION_MAX 0x3

#define PACKAGE2_MINVER_THEORETICAL 0x0
#define PACKAGE2_MAXVER_100 0x2
#define PACKAGE2_MAXVER_200 0x3
#define PACKAGE2_MAXVER_300 0x4
#define PACKAGE2_MAXVER_302 0x5
#define PACKAGE2_MAXVER_400_410 0x6
#define PACKAGE2_MAXVER_500_510 0x7
#define PACKAGE2_MAXVER_600_610 0x8
#define PACKAGE2_MAXVER_620 0x9
#define PACKAGE2_MAXVER_700_800 0xA
#define PACKAGE2_MAXVER_810 0xB
#define PACKAGE2_MAXVER_900 0xC
#define PACKAGE2_MAXVER_910_920 0xD
#define PACKAGE2_MAXVER_1000 0xE
#define PACKAGE2_MAXVER_1100_CURRENT 0xF

#define PACKAGE2_MINVER_100 0x3
#define PACKAGE2_MINVER_200 0x4
#define PACKAGE2_MINVER_300 0x5
#define PACKAGE2_MINVER_302 0x6
#define PACKAGE2_MINVER_400_410 0x7
#define PACKAGE2_MINVER_500_510 0x8
#define PACKAGE2_MINVER_600_610 0x9
#define PACKAGE2_MINVER_620 0xA
#define PACKAGE2_MINVER_700_800 0xB
#define PACKAGE2_MINVER_810 0xC
#define PACKAGE2_MINVER_900 0xD
#define PACKAGE2_MINVER_910_920 0xE
#define PACKAGE2_MINVER_1000 0xF
#define PACKAGE2_MINVER_1100_CURRENT 0x10

#define NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS ((void *)(0xA9800000ull))

typedef struct {
    union {
        uint8_t ctr[0x10];
        uint32_t ctr_dwords[0x4];
    };
    uint8_t section_ctrs[4][0x10];
    uint32_t magic;
    uint32_t entrypoint;
    uint32_t _0x58;
    uint8_t version_max; /* Must be > TZ value. */
    uint8_t version_min; /* Must be < TZ value. */
    uint16_t _0x5E;
    uint32_t section_sizes[4];
    uint32_t section_offsets[4];
    uint8_t section_hashes[4][0x20];
} package2_meta_t;

typedef struct {
    uint8_t signature[0x100];
    union {
        package2_meta_t metadata;
        uint8_t encrypted_header[0x100];
    };
    uint8_t data[];
} package2_header_t;

/* Package2 can be encrypted or unencrypted for these functions: */

static inline size_t package2_meta_get_size(const package2_meta_t *metadata) {
    return metadata->ctr_dwords[0] ^ metadata->ctr_dwords[2] ^ metadata->ctr_dwords[3];
}

static inline uint8_t package2_meta_get_header_version(const package2_meta_t *metadata) {
    return (uint8_t)((metadata->ctr_dwords[1] ^ (metadata->ctr_dwords[1] >> 16) ^ (metadata->ctr_dwords[1] >> 24)) & 0xFF);
}

void package2_rebuild_and_copy(package2_header_t *package2, uint32_t target_firmware, void *mesosphere, size_t mesosphere_size, void *emummc, size_t emummc_size);

#endif
