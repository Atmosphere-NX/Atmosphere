/*
 * Copyright (c) 2019 Atmosphère-NX
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

#pragma once

#include "utils.h"

#define MEMORY_MAP_MEMTYPE_DEVICE_NGNRNE                0ul
#define MEMORY_MAP_MEMTYPE_NORMAL                       1ul
#define MEMORY_MAP_MEMTYPE_DEVICE_NGNRE                 2ul
#define MEMORY_MAP_MEMTYPE_NORMAL_UNCACHEABLE           3ul

#define MEMORY_MAP_VA_SPACE_SIZE                        39ul

// The following few definitions depend on the value of MEMORY_MAP_VA_SPACE_SIZE:
#define MEMORY_MAP_SELF_L2_VA_RANGE                     0x7FC0000000ul // = 511 << 31
#define MEMORY_MAP_SELF_L3_VA_RANGE                     0x7FFFE00000ul // = 511 << 31 | 511 << 21
#define MEMORY_MAP_VA_TTBL                              0x7FFFFFF000ul // = 511 << 31 | 511 << 21 | 511 << 12
#define MEMORY_MAP_VA_MAX                               0x7FFFFFFFFFul // = all 39 bits set

#define MEMORY_MAP_VA_CRASH_STACKS_SIZE                 0x1000ul
#define MEMORY_MAP_VA_IMAGE                             (MEMORY_MAP_SELF_L3_VA_RANGE + 0x10000)
#define MEMORY_MAP_VA_CRASH_STACKS_BOTTOM               (MEMORY_MAP_SELF_L3_VA_RANGE + 0x40000)
#define MEMORY_MAP_VA_CRASH_STACKS_TOP                  (MEMORY_MAP_SELF_L3_VA_RANGE + 0x41000)
#define MEMORY_MAP_VA_GUEST_MEM                         (MEMORY_MAP_SELF_L3_VA_RANGE + 0x50000)
#define MEMORY_MAP_VA_STACKS_TOP                        (MEMORY_MAP_SELF_L3_VA_RANGE + 0x80000)

#define MEMORY_MAP_VA_MMIO_BASE                         MEMORY_MAP_VA_STACKS_TOP
#define MEMORY_MAP_VA_GICD                              MEMORY_MAP_VA_MMIO_BASE
#define MEMORY_MAP_VA_GICC                              (MEMORY_MAP_VA_MMIO_BASE + 0x1000)
#define MEMORY_MAP_VA_GICH                              (MEMORY_MAP_VA_MMIO_BASE + 0x3000)

#define MEMORY_MAP_VA_MMIO_PLAT_BASE                    (MEMORY_MAP_VA_MMIO_BASE + 0x90000)


typedef struct LoadImageLayout {
    uintptr_t startPa;
    size_t maxImageSize;
    size_t imageSize; // "image" includes "real" BSS but not tempbss

    uintptr_t tempPa;
    size_t maxTempSize;
    size_t tempSize;

    uintptr_t vbar;
} LoadImageLayout;

extern LoadImageLayout g_loadImageLayout;

// Non-reentrant
uintptr_t memoryMapPlatformMmio(uintptr_t pa, size_t size);
