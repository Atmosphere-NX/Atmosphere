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
 
#ifndef EXOSPHERE_MMU_H
#define EXOSPHERE_MMU_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "utils.h"


#ifndef MMU_GRANULE_TYPE
#define MMU_GRANULE_TYPE    0 /* 0: 4KB, 1: 64KB, 2: 16KB. The Switch always uses a 4KB granule size. */
#endif

#if MMU_GRANULE_TYPE == 0
#define MMU_Lx_SHIFT(x) (12 + 9 * (3 - (x)))
#define MMU_Lx_MASK(x)  MASKL(9)
#elif MMU_GRANULE_TYPE == 1
/* 64 KB, no L0 here */
#define MMU_Lx_SHIFT(x) (16 + 13 * (3 - (x)))
#define MMU_Lx_MASK(x)  ((x) == 1 ? MASKL(5) : MASKL(13))
#elif MMU_GRANULE_TYPE == 2
#define MMU_Lx_SHIFT(x) (14 + 11 * (3 - (x)))
#define MMU_Lx_MASK(x)  ((x) == 0 ? 1 : MASKL(11))
#endif

/*
 * The following defines are adapted from uboot:
 *
 * (C) Copyright 2013
 * David Feng <fenghua@phytium.com.cn>
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

/* Memory attributes, see set_memory_registers_enable_mmu */
#define MMU_MT_NORMAL           0ull
#define MMU_MT_DEVICE_NGNRE     1ull
#define MMU_MT_DEVICE_NGNRNE    2ull /* not used, also the same as Attr4-7 */

/*
 * Hardware page table definitions.
 *
 */

#define MMU_PTE_TYPE_MASK       3ull
#define MMU_PTE_TYPE_FAULT      0ull
#define MMU_PTE_TYPE_TABLE      3ull
#define MMU_PTE_TYPE_BLOCK      1ull

/* L3 only */
#define MMU_PTE_TYPE_PAGE       3ull

#define MMU_PTE_TABLE_PXN       BITL(59)
#define MMU_PTE_TABLE_XN        BITL(60)
#define MMU_PTE_TABLE_AP        BITL(61)
#define MMU_PTE_TABLE_NS        BITL(63)

/*
 * Block
 */
#define MMU_PTE_BLOCK_MEMTYPE(x)        ((uint64_t)((x) << 2))
#define MMU_PTE_BLOCK_NS                BITL(5)
#define MMU_PTE_BLOCK_NON_SHAREABLE     (0ull << 8)
#define MMU_PTE_BLOCK_OUTER_SHAREABLE   (2ull << 8)
#define MMU_PTE_BLOCK_INNER_SHAREBLE    (3ull << 8)
#define MMU_PTE_BLOCK_AF                BITL(10)
#define MMU_PTE_BLOCK_NG                BITL(11)
#define MMU_PTE_BLOCK_PXN               BITL(53)
#define MMU_PTE_BLOCK_UXN               BITL(54)
#define MMU_PTE_BLOCK_XN                MMU_PTE_BLOCK_UXN

/*
 * AP[2:1]
 */
#define MMU_AP_PRIV_RW                  (0ull << 6)
#define MMU_AP_RW                       (1ull << 6)
#define MMU_AP_PRIV_RO                  (2ull << 6)
#define MMU_AP_RO                       (3ull << 6)

/*
 * S2AP[2:1] (for stage2 translations; secmon doesn't use it)
 */
#define MMU_S2AP_NONE                   (0ull << 6)
#define MMU_S2AP_RO                     (1ull << 6)
#define MMU_S2AP_WO                     (2ull << 6)
#define MMU_S2AP_RW                     (3ull << 6)

/*
 * AttrIndx[2:0]
 */
#define MMU_PMD_ATTRINDX(t)     ((uint64_t)((t) << 2))
#define MMU_PMD_ATTRINDX_MASK   (7ull << 2)

/*
 * TCR flags.
 */
#define TCR_T0SZ(x)         ((64 - (x)) << 0)
#define TCR_IRGN_NC         (0 << 8)
#define TCR_IRGN_WBWA       (1 << 8)
#define TCR_IRGN_WT         (2 << 8)
#define TCR_IRGN_WBNWA      (3 << 8)
#define TCR_IRGN_MASK       (3 << 8)
#define TCR_ORGN_NC         (0 << 10)
#define TCR_ORGN_WBWA       (1 << 10)
#define TCR_ORGN_WT         (2 << 10)
#define TCR_ORGN_WBNWA      (3 << 10)
#define TCR_ORGN_MASK       (3 << 10)
#define TCR_NOT_SHARED      (0 << 12)
#define TCR_SHARED_OUTER    (2 << 12)
#define TCR_SHARED_INNER    (3 << 12)
#define TCR_TG0_4K          (0 << 14)
#define TCR_TG0_64K         (1 << 14)
#define TCR_TG0_16K         (2 << 14)
#define TCR_PS(x)           ((x) << 16)
#define TCR_EPD1_DISABLE    BIT(23)

#define TCR_EL1_RSVD        BIT(31)
#define TCR_EL2_RSVD        (BIT(31) | BIT(23))
#define TCR_EL3_RSVD        (BIT(31) | BIT(23))

static inline void mmu_init_table(uintptr_t *tbl, size_t num_entries) {
    for(size_t i = 0; i < num_entries / 8; i++) {
        tbl[i] = MMU_PTE_TYPE_FAULT;
    }
}

/*
    All the functions below assume base_addr is valid.
    They do not invalidate the TLB, which must be done separately.
*/

static inline unsigned int mmu_compute_index(unsigned int level, uintptr_t base_addr) {
    return (base_addr >> MMU_Lx_SHIFT(level)) & MMU_Lx_MASK(level);
}

static inline void mmu_map_table(unsigned int level, uintptr_t *tbl, uintptr_t base_addr, uintptr_t *next_lvl_tbl_pa, uint64_t attrs) {
    tbl[mmu_compute_index(level, base_addr)] = (uintptr_t)next_lvl_tbl_pa | attrs | MMU_PTE_TYPE_TABLE;
}

static inline void mmu_map_block(unsigned int level, uintptr_t *tbl, uintptr_t base_addr, uintptr_t phys_addr, uint64_t attrs) {
    tbl[mmu_compute_index(level, base_addr)] = phys_addr | attrs | MMU_PTE_BLOCK_AF | MMU_PTE_TYPE_BLOCK;
}

static inline void mmu_map_page(uintptr_t *tbl, uintptr_t base_addr, uintptr_t phys_addr, uint64_t attrs) {
    tbl[mmu_compute_index(3, base_addr)] = phys_addr | attrs | MMU_PTE_BLOCK_AF | MMU_PTE_TYPE_PAGE;
}

static inline void mmu_unmap(unsigned int level, uintptr_t *tbl, uintptr_t base_addr) {
    tbl[mmu_compute_index(level, base_addr)] = MMU_PTE_TYPE_FAULT;
}

static inline void mmu_unmap_page(uintptr_t *tbl, uintptr_t base_addr) {
    tbl[mmu_compute_index(3, base_addr)] = MMU_PTE_TYPE_FAULT;
}

static inline void mmu_map_block_range(unsigned int level, uintptr_t *tbl, uintptr_t base_addr, uintptr_t phys_addr, size_t size, uint64_t attrs) {
    size = ((size + (BITL(MMU_Lx_SHIFT(level)) - 1)) >> MMU_Lx_SHIFT(level)) << MMU_Lx_SHIFT(level);
    for(size_t offset = 0; offset < size; offset += BITL(MMU_Lx_SHIFT(level))) {
        mmu_map_block(level, tbl, base_addr + offset, phys_addr + offset, attrs);
    }
}

static inline void mmu_map_page_range(uintptr_t *tbl, uintptr_t base_addr, uintptr_t phys_addr, size_t size, uint64_t attrs) {
    size = ((size + (BITL(MMU_Lx_SHIFT(3)) - 1)) >> MMU_Lx_SHIFT(3)) << MMU_Lx_SHIFT(3);
    for(size_t offset = 0; offset < size; offset += BITL(MMU_Lx_SHIFT(3))) {
        mmu_map_page(tbl, base_addr + offset, phys_addr + offset, attrs);
    }
}

static inline void mmu_unmap_range(unsigned int level, uintptr_t *tbl, uintptr_t base_addr, size_t size) {
    size = ((size + (BITL(MMU_Lx_SHIFT(level)) - 1)) >> MMU_Lx_SHIFT(level)) << MMU_Lx_SHIFT(level);
    for(size_t offset = 0; offset < size; offset += BITL(MMU_Lx_SHIFT(level))) {
        mmu_unmap(level, tbl, base_addr + offset);
    }
}

#endif
