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
#define MMU_PTE_BLOCK_XN                MMU_PTE_BLOCK_UXN

/*
 * AP[2:1]
 */
#define MMU_AP_PRIV_RW                  (0 << 6)
#define MMU_AP_RW                       (1 << 6)
#define MMU_AP_PRIV_RO                  (2 << 6)
#define MMU_AP_RO                       (3 << 6)

/*
 * S2AP[2:1] (for stage2 translations; secmon doesn't use it)
 */
#define MMU_S2AP_NONE                   (0 << 6)
#define MMU_S2AP_RO                     (1 << 6)
#define MMU_S2AP_WO                     (2 << 6)
#define MMU_S2AP_RW                     (3 << 6)

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
#define TCR_EPD1_DISABLE    BIT(23)

#define TCR_EL1_RSVD        BIT(31)
#define TCR_EL2_RSVD        (BIT(31) | BIT(23))
#define TCR_EL3_RSVD        (BIT(31) | BIT(23))

static inline void mmu_init_table(uintptr_t *tbl, size_t num_entries) {
    for(size_t i = 0; i < num_entries; i++) {
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

static inline void mmu_map_block_range(unsigned int level, uintptr_t *tbl, uintptr_t base_addr, uintptr_t phys_addr, size_t size, uint64_t attrs) {
    size = (size >> MMU_Lx_SHIFT(level)) << MMU_Lx_SHIFT(level);
    for(size_t offset = 0; offset < size; offset += MMU_Lx_SHIFT(level)) {
        mmu_map_block(level, tbl, base_addr + offset, phys_addr + offset, attrs);
    }
}

static inline void mmu_map_page_range(uintptr_t *tbl, uintptr_t base_addr, uintptr_t phys_addr, size_t size, uint64_t attrs) {
    size = (size >> MMU_Lx_SHIFT(3)) << MMU_Lx_SHIFT(3);
    for(size_t offset = 0; offset < size; offset += MMU_Lx_SHIFT(3)) {
        mmu_map_page(tbl, base_addr + offset, phys_addr + offset, attrs);
    }
}

static inline void mmu_unmap_range(unsigned int level, uintptr_t *tbl, uintptr_t base_addr, uintptr_t phys_addr, size_t size) {
    size = (size >> MMU_Lx_SHIFT(level)) << MMU_Lx_SHIFT(level);
    for(size_t offset = 0; offset < size; offset += MMU_Lx_SHIFT(level)) {
        mmu_unmap(level, tbl, base_addr + offset, phys_addr + offset);
    }
}

/* Switch specific stuff */

static const struct {
    uintptr_t pa;
    size_t size;
    bool is_secure;
} devices[] =
{
    { 0x50041000, 0x1000, true  }, /* ARM Interrupt Distributor */
    { 0x50042000, 0x2000, true  }, /* Interrupt Controller Physical CPU interface */
    { 0x70006000, 0x1000, false }, /* UART-A */
    { 0x60006000, 0x1000, false }, /* Clock and Reset */
    { 0x7000E000, 0x1000, true  }, /* RTC, PMC */
    { 0x60005000, 0x1000, true  }, /* TMRs, WDTs */
    { 0x6000C000, 0x1000, true  }, /* System Registers */
    { 0x70012000, 0x2000, true  }, /* SE */
    { 0x700F0000, 0x1000, true  }, /* SYSCTR0 */
    { 0x70019000, 0x1000, true  }, /* MC */
    { 0x7000F000, 0x1000, true  }, /* FUSE (0x7000F800) */
    { 0x70000000, 0x4000, true  }, /* MISC */
    { 0x60007000, 0x1000, true  }, /* Flow Controller */
    { 0x40002000, 0x1000, true  }, /* iRAM-A */
    { 0x7000D000, 0x1000, true  }, /* I2C-5,6 - SPI 2B-1 to 4 */
    { 0x6000D000, 0x1000, true  }, /* GPIO-1 - GPIO-8 */
    { 0x7000C000, 0x1000, true  }, /* I2C-I2C4 */
    { 0x6000F000, 0x1000, true  }, /* Exception vectors */
};

#define MMIO_DEVID_GICD                 0
#define MMIO_DEVID_ICC                  1
#define MMIO_DEVID_UART_A               2
#define MMIO_DEVID_CLKRST               3
#define MMIO_DEVID_RTC_PMC              4
#define MMIO_DEVID_TMRs_WDTs            5
#define MMIO_DEVID_SYSREGS              6
#define MMIO_DEVID_SE                   7
#define MMIO_DEVID_SYSCTR0              8
#define MMIO_DEVID_MC                   9
#define MMIO_DEVID_FUSE                 10
#define MMIO_DEVID_MISC                 11
#define MMIO_DEVID_FLOWCTRL             12
#define MMIO_DEVID_NXBOOTLOADER_MAILBOX 13
#define MMIO_DEVID_I2C56_SPI2B          14
#define MMIO_DEVID_GPIO                 15
#define MMIO_DEVID_DTV_I2C234           16
#define MMIO_DEVID_EXCEPTION_VECTORS    17

#ifndef MMIO_USE_IDENTIY_MAPPING
#define MMIO_BASE   0x1F0080000ull

static inline uintptr_t mmio_get_device_address(unsigned int device_id) {
    size_t offset = 0;
    for(unsigned int i = 0; i < device_id; i++) {
        offset += devices[i].size;
        offset += 0x1000; /* guard page */
    }

    return MMIO_BASE + offset;
}

#else
static inline uintptr_t mmio_get_device_address(unsigned int devid) {
    return devices[devid];
}
#endif

static inline void mmio_map_all_devices(uintptr_t *mmu_l3_tbl) {
    static const uint64_t secure_device_attributes  = MMU_PTE_BLOCK_XN | MMU_PTE_BLOCK_INNER_SHAREBLE | MMU_PTE_BLOCK_MEMTYPE(1);
    static const uint64_t device_attributes         = MMU_PTE_TABLE_NS | secure_device_attributes;

    for(size_t i = 0, offset = 0; i < sizeof(devices) / sizeof(devices[0]); i++) {
        uint64_t attributes = devices[i].is_secure ? secure_device_attributes : device_attributes;
        mmu_map_page_range(mmu_l3_tbl, MMIO_BASE + offset, devices[i].pa, devices[i].size, attributes);

        offset += devices[i].size;
        offset += 0x1000; /* insert guard page */
    }
}

static inline void mmio_unmap_all_devices(uintptr_t *mmu_l3_tbl) {
    for(size_t i = 0, offset = 0; i < sizeof(devices) / sizeof(devices[0]); i++) {
        mmu_unmap_page_range(mmu_l3_tbl, MMIO_BASE + offset, devices[i].pa, devices[i].size);

        offset += devices[i].size;
        offset += 0x1000; /* insert guard page */
    }
}

#endif
