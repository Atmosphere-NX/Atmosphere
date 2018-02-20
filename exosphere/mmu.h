#ifndef EXOSPHERE_MMU_H
#define EXOSPHERE_MMU_H

#include <stdint.h>
#include <stddef.h>
#include "utils.h"


#ifndef MMU_GRANULE_TYPE
#define MMU_GRANULE_TYPE    0 /* 0: 4KB, 1: 64KB, 2: 16KB. The Switch always uses a 4KB granule size. */
#endif

#if MMU_GRANULE_TYPE == 0
#define MMU_Lx_SHIFT(x) (12 + 9 * (3 - (x)))
#define MMU_Lx_MASK(x)  (BIT(9) - 1)
#elif MMU_GRANULE_TYPE == 1
/* 64 KB, no L0 here */
#define MMU_Lx_SHIFT(x) (16 + 13 * (3 - (x)))
#define MMU_Lx_MASK(x)  ((x) == 1 ? (BIT(5) - 1) : (BIT(13) - 1))
#elif MMU_GRANULE_TYPE == 2
#define MMU_Lx_SHIFT(x) (14 + 11 * (3 - (x)))
#define MMU_Lx_MASK(x)  ((x) == 0 ? 1 : (BIT(11) - 1))
#endif

/*
 * The following defines are adapted from uboot:
 *
 * (C) Copyright 2013
 * David Feng <fenghua@phytium.com.cn>
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#define MMU_MT_DEVICE_NGNRNE    0
#define MMU_MT_DEVICE_NGNRE     1
#define MMU_MT_DEVICE_GRE       2
#define MMU_MT_NORMAL_NC        3
#define MMU_MT_NORMAL           4

/*
 * Hardware page table definitions.
 *
 */

#define MMU_PTE_TYPE_MASK       3
#define MMU_PTE_TYPE_FAULT      0
#define MMU_PTE_TYPE_TABLE      3
#define MMU_PTE_TYPE_BLOCK      1

/* L3 only */
#define MMU_PTE_TYPE_PAGE       1

#define MMU_PTE_TABLE_PXN       BITL(59)
#define MMU_PTE_TABLE_XN        BITL(60)
#define MMU_PTE_TABLE_AP        BITL(61)
#define MMU_PTE_TABLE_NS        BITL(63)

/*
 * Block
 */
#define MMU_PTE_BLOCK_MEMTYPE(x)        ((x) << 2)
#define MMU_PTE_BLOCK_NS                BIT(5)
#define MMU_PTE_BLOCK_NON_SHAREABLE     (0 << 8)
#define MMU_PTE_BLOCK_OUTER_SHAREABLE   (2 << 8)
#define MMU_PTE_BLOCK_INNER_SHAREBLE    (3 << 8)
#define MMU_PTE_BLOCK_AF                BIT(10)
#define MMU_PTE_BLOCK_NG                BIT(11)
#define MMU_PTE_BLOCK_PXN               BITL(53)
#define MMU_PTE_BLOCK_UXN               BITL(54)
#define MMU_PTE_BLOCK_UXN               MMU_PTE_BLOCK_UXN

/*
 * AttrIndx[2:0]
 */
#define MMU_PMD_ATTRINDX(t)     ((t) << 2)
#define MMU_PMD_ATTRINDX_MASK   (7 << 2)

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
#define TCR_ORGN_WBNWA      (1 << 10)
#define TCR_ORGN_WT         (2 << 10)
#define TCR_ORGN_WBNWA      (3 << 10)
#define TCR_ORGN_MASK       (3 << 10)
#define TCR_NOT_SHARED      (0 << 12)
#define TCR_SHARED_OUTER    (2 << 12)
#define TCR_SHARED_INNER    (3 << 12)
#define TCR_TG0_4K          (0 << 14)
#define TCR_TG0_64K         (1 << 14)
#define TCR_TG0_16K         (2 << 14)
#define TCR_EPD1_DISABLE    BIT(23)

#define TCR_EL1_RSVD        BIT(31)
#define TCR_EL2_RSVD        (BIT(31) | BIT(23))
#define TCR_EL3_RSVD        (BIT(31) | BIT(23))

static inline void mmu_init_table(unsigned int level, uintptr_t *tbl) {
    for(size_t i = 0; i < MMU_Lx_MASK(level); i++) {
        tbl[i] = MMU_PTE_TYPE_FAULT;
    }
}

/*
    All the functions below assume base_addr is valid.
    They do not invalidate the TLB, which must be done separately.
*/

static inline void mmu_compute_index(unsigned int level, uintptr_t base_addr) {
    return (base_addr >> MMU_Lx_SHIFT(level)) & MMU_Lx_MASK(level);
}

static inline void mmu_map_table(unsigned int level, uintptr_t *tbl, uintptr_t base_addr, uintptr_t *next_lvl_tbl_pa, uint64_t attrs) {
    tbl[mmu_compute_index(level, base_addr)] = (uintptr_t)next_lvl_tbl_pa | attrs | MMU_PTE_TYPE_TABLE;
}

static inline void mmu_map_block_l012(unsigned int level, uintptr_t *tbl, uintptr_t base_addr, uintptr_t phys_addr, uint64_t attrs) {
    tbl[mmu_compute_index(level, base_addr)] = phys_addr | attrs | MMU_PTE_TYPE_BLOCK;
}

static inline void mmu_map_page(uintptr_t *tbl, uintptr_t base_addr, uintptr_t phys_addr, uint64_t attrs) {
    tbl[mmu_compute_index(3, base_addr)] = phys_addr | attrs | MMU_PTE_BLOCK_AF | MMU_PTE_TYPE_PAGE;
}

static inline void mmu_unmap(unsigned int level, uintptr_t *tbl, uintptr_t base_addr) {
    tbl[mmu_compute_index(level, base_addr)] = MMU_PTE_TYPE_FAULT;
}

static inline void mmu_map_block_range_l012(unsigned int level, uintptr_t *tbl, uintptr_t base_addr, uintptr_t phys_addr, size_t size, uint64_t attrs) {
    size = (size >> MMU_Lx_SHIFT(level)) << MMU_Lx_SHIFT(level);
    for(size_t offset = 0; offset < size; offset += MMU_Lx_SHIFT(level)) {
        mmu_map_block_l012(level, tbl, base_addr + offset, phys_addr + offset, attrs);
    }
}

static inline void mmu_map_page_range(uintptr_t *tbl, uintptr_t base_addr, uintptr_t phys_addr, size_t size, uint64_t attrs) {
    size = (size >> MMU_Lx_SHIFT(3)) << MMU_Lx_SHIFT(3);
    for(size_t offset = 0; offset < size; offset += MMU_Lx_SHIFT(3)) {
        mmu_map_page(base_addr + offset, phys_addr + offset, attrs);
    }
}

static inline void mmu_unmap_range(unsigned int level, uintptr_t *tbl, uintptr_t base_addr, uintptr_t phys_addr, size_t size) {
    size = (size >> MMU_Lx_SHIFT(level)) << MMU_Lx_SHIFT(level);
    for(size_t offset = 0; offset < size; offset += MMU_Lx_SHIFT(level)) {
        mmu_unmap(level, tbl, base_addr + offset, phys_addr + offset);
    }
}

#endif
