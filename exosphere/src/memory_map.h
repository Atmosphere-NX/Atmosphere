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
 
#ifndef EXOSPHERE_MEMORY_MAP_H
#define EXOSPHERE_MEMORY_MAP_H

#include "mmu.h"
#include "preprocessor.h"

#define ATTRIB_MEMTYPE_NORMAL MMU_PTE_BLOCK_MEMTYPE(MMU_MT_NORMAL)
#define ATTRIB_MEMTYPE_DEVICE MMU_PTE_BLOCK_MEMTYPE(MMU_MT_DEVICE_NGNRE)

/* Identity mappings (addr, size, additional attributes, is block range) */
#define _MMAPID0   ( 0x40006000ull, 0x01000ull,                                   0ull, false ) /* Part of iRAM-A, where our coldboot crt0 is */
#define _MMAPID1   ( 0x40020000ull, 0x20000ull,                                   0ull, false ) /* iRAM-C+D */
#define _MMAPID2   ( 0x7C010000ull, 0x10000ull,                                   0ull, false ) /* TZRAM (contains the secmon's warmboot crt0) */
#define _MMAPID3   ( 0x80000000ull, 4ull << 30,    MMU_PTE_BLOCK_XN | MMU_PTE_BLOCK_NS,  true ) /* DRAM (4GB) */

/* MMIO (addr, size, is secure) */
#define _MMAPDEV0       ( 0x50041000ull, 0x1000ull, true  ) /* ARM Interrupt Distributor */
#define _MMAPDEV1       ( 0x50042000ull, 0x2000ull, true  ) /* Interrupt Controller Physical CPU interface */
#define _MMAPDEV2       ( 0x70006000ull, 0x1000ull, false ) /* UART */
#define _MMAPDEV3       ( 0x60006000ull, 0x1000ull, false ) /* Clock and Reset */
#define _MMAPDEV4       ( 0x7000E000ull, 0x1000ull, true  ) /* RTC, PMC */
#define _MMAPDEV5       ( 0x60005000ull, 0x1000ull, true  ) /* TMRs, WDTs */
#define _MMAPDEV6       ( 0x6000C000ull, 0x1000ull, true  ) /* System Registers */
#define _MMAPDEV7       ( 0x70012000ull, 0x2000ull, true  ) /* SE */
#define _MMAPDEV8       ( 0x700F0000ull, 0x1000ull, true  ) /* SYSCTR0 */
#define _MMAPDEV9       ( 0x70019000ull, 0x1000ull, true  ) /* MC */
#define _MMAPDEV10      ( 0x7000F000ull, 0x1000ull, true  ) /* FUSE (0x7000F800) */
#define _MMAPDEV11      ( 0x70000000ull, 0x4000ull, true  ) /* MISC */
#define _MMAPDEV12      ( 0x60007000ull, 0x1000ull, true  ) /* Flow Controller */
#define _MMAPDEV13      ( 0x40002000ull, 0x1000ull, true  ) /* NX bootloader mailbox page */
#define _MMAPDEV14      ( 0x7000D000ull, 0x1000ull, true  ) /* I2C-5,6 - SPI 2B-1 to 4 */
#define _MMAPDEV15      ( 0x6000D000ull, 0x1000ull, true  ) /* GPIO-1 - GPIO-8 */
#define _MMAPDEV16      ( 0x7000C000ull, 0x1000ull, true  ) /* I2C-I2C4 */
#define _MMAPDEV17      ( 0x6000F000ull, 0x1000ull, true  ) /* Exception vectors */
#define _MMAPDEV18      ( 0x40038000ull, 0x8000ull, true  ) /* DEBUG: IRAM */

/* LP0 entry ram segments (addr, size, additional attributes) */
#define _MMAPLP0ES0  ( 0x40020000ull, 0x10000ull, MMU_PTE_BLOCK_NS | ATTRIB_MEMTYPE_DEVICE ) /* Encrypted TZRAM */
#define _MMAPLP0ES1  ( 0x40003000ull, 0x01000ull, MMU_PTE_BLOCK_NS | ATTRIB_MEMTYPE_DEVICE ) /* LP0 entry code */
#define _MMAPLP0ES2  ( 0x7C010000ull, 0x10000ull, MMU_AP_PRIV_RO   | ATTRIB_MEMTYPE_NORMAL ) /* TZRAM to encrypt */

/* Warmboot data ram segments (addr, size, additional attributes) */
#define _MMAPWBS0   ( 0x8000F000ull, 0x01000ull, MMU_PTE_BLOCK_NS | ATTRIB_MEMTYPE_DEVICE ) /* Encrypted SE state for bootROM */
#define _MMAPWBS1   ( 0x80010000ull, 0x10000ull, MMU_PTE_BLOCK_NS | ATTRIB_MEMTYPE_DEVICE ) /* Encrypted TZRAM for warmboot.bin */

/* TZRAM segments (offset, size, VA increment, is executable) */
#define _MMAPTZS0   (           0x3000ull, 0x10000 - 0x2000 - 0x3000ull, 0x10000ull,  true ) /* Warmboot crt0 sections and main code segment */
#define _MMAPTZS1   ( 0x10000 - 0x2000ull,                    0x2000ull, 0x04000ull,  true ) /* pk2ldr segment */
#define _MMAPTZS2   (                0ull,                         0ull, 0x02000ull, false ) /* SPL .bss buffer, NOT mapped at startup */
#define _MMAPTZS3   ( 0x10000 - 0x2000ull,                    0x1000ull, 0x02000ull, false ) /* Core 0ull1,2 stack */
#define _MMAPTZS4   ( 0x10000 - 0x1000ull,                    0x1000ull, 0x02000ull, false ) /* Core 3 stack */
#define _MMAPTZS5   (                0ull,                    0x1000ull, 0x02000ull,  true ) /* Secure Monitor exception vectors, some init stacks */
#define _MMAPTZS6   (           0x1000ull,                    0x1000ull, 0x02000ull, false ) /* L2 translation table */
#define _MMAPTZS7   (           0x2000ull,                    0x1000ull, 0x02000ull, false ) /* L3 translation table */

/* TZRAM segments for 5.0.0+. (offset).  */
#define _MMAPTZ5XS0   (           0x3000ull ) /* Warmboot crt0 sections and main code segment */
#define _MMAPTZ5XS1   (                0ull ) /* pk2ldr segment */
#define _MMAPTZ5XS2   (                0ull ) /* SPL .bss buffer, NOT mapped at startup */
#define _MMAPTZ5XS3   (                0ull ) /* Core 0ull1,2 stack */
#define _MMAPTZ5XS4   (           0x1000ull ) /* Core 3 stack */
#define _MMAPTZ5XS5   (           0x2000ull ) /* Secure Monitor exception vectors, some init stacks */
#define _MMAPTZ5XS6   ( 0x10000 - 0x2000ull ) /* L2 translation table */
#define _MMAPTZ5XS7   ( 0x10000 - 0x1000ull ) /* L3 translation table */

#define MMIO_BASE                       0x1F0080000ull
#define LP0_ENTRY_RAM_SEGMENT_BASE      (MMIO_BASE + 0x000100000ull)
#define WARMBOOT_RAM_SEGMENT_BASE       (LP0_ENTRY_RAM_SEGMENT_BASE + 0x000047000ull) /* increment seems to be arbitrary ? */
#define TZRAM_SEGMENT_BASE              (MMIO_BASE + 0x000160000ull)

#define IDENTITY_MAPPING_IRAM_A_CCRT0   0
#define IDENTITY_MAPPING_IRAM_CD        1
#define IDENTITY_MAPPING_TZRAM          2
#define IDENTITY_MAPPING_DRAM           3
#define IDENTIY_MAPPING_ID_MAX          4

#define MMIO_DEVID_GICD                 0
#define MMIO_DEVID_GICC                 1
#define MMIO_DEVID_UART                 2
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
#define MMIO_DEVID_DEBUG_IRAM           18
#define MMIO_DEVID_MAX                  19

#define LP0_ENTRY_RAM_SEGMENT_ID_ENCRYPTED_TZRAM    0
#define LP0_ENTRY_RAM_SEGMENT_ID_LP0_ENTRY_CODE     1
#define LP0_ENTRY_RAM_SEGMENT_ID_CURRENT_TZRAM      2
#define LP0_ENTRY_RAM_SEGMENT_ID_MAX                3

#define WARMBOOT_RAM_SEGMENT_ID_SE_STATE            0
#define WARMBOOT_RAM_SEGMENT_ID_TZRAM               1
#define WARMBOOT_RAM_SEGMENT_ID_MAX                 2

#define TZRAM_SEGMENT_ID_WARMBOOT_CRT0_AND_MAIN     0
#define TZRAM_SEGMENT_ID_PK2LDR                     1
#define TZRAM_SEGMENT_ID_USERPAGE                   2
#define TZRAM_SEGMENT_ID_CORE012_STACK              3
#define TZRAM_SEGMENT_ID_CORE3_STACK                4
#define TZRAM_SEGEMENT_ID_SECMON_EVT                5
#define TZRAM_SEGMENT_ID_L2_TRANSLATION_TABLE       6
#define TZRAM_SEGMENT_ID_L3_TRANSLATION_TABLE       7
#define TZRAM_SEGMENT_ID_MAX                        8

#define IDENTITY_GET_MAPPING_ADDRESS(mapping_id)        (TUPLE_ELEM_0(CAT(_MMAPID, EVAL(mapping_id))))
#define IDENTITY_GET_MAPPING_SIZE(mapping_id)           (TUPLE_ELEM_1(CAT(_MMAPID, EVAL(mapping_id))))
#define IDENTITY_GET_MAPPING_ATTRIBS(mapping_id)        (TUPLE_ELEM_2(CAT(_MMAPID, EVAL(mapping_id))))
#define IDENTITY_IS_MAPPING_BLOCK_RANGE(mapping_id)     (TUPLE_ELEM_3(CAT(_MMAPID, EVAL(mapping_id))))

#define MMIO_GET_DEVICE_PA(device_id)                   (TUPLE_ELEM_0(CAT(_MMAPDEV, EVAL(device_id))))
#define MMIO_GET_DEVICE_ADDRESS(device_id)\
(\
    (TUPLE_FOLD_LEFT_1(EVAL(device_id), _MMAPDEV, PLUS) EVAL(MMIO_BASE)) +\
    0x1000ull * (device_id)\
)
#define MMIO_GET_DEVICE_SIZE(device_id)                 (TUPLE_ELEM_1(CAT(_MMAPDEV, EVAL(device_id))))
#define MMIO_IS_DEVICE_SECURE(device_id)                (TUPLE_ELEM_2(CAT(_MMAPDEV, EVAL(device_id))))

#define LP0_ENTRY_GET_RAM_SEGMENT_PA(segment_id)        (TUPLE_ELEM_0(CAT(_MMAPLP0ES, EVAL(segment_id))))
#define LP0_ENTRY_GET_RAM_SEGMENT_ADDRESS(segment_id)   (EVAL(LP0_ENTRY_RAM_SEGMENT_BASE) + 0x10000ull * (segment_id))
#define LP0_ENTRY_GET_RAM_SEGMENT_SIZE(segment_id)      (TUPLE_ELEM_1(CAT(_MMAPLP0ES, EVAL(segment_id))))
#define LP0_ENTRY_GET_RAM_SEGMENT_ATTRIBS(segment_id)   (TUPLE_ELEM_2(CAT(_MMAPLP0ES, EVAL(segment_id))))

#define WARMBOOT_GET_RAM_SEGMENT_PA(segment_id)         (TUPLE_ELEM_0(CAT(_MMAPWBS, EVAL(segment_id))))
#define WARMBOOT_GET_RAM_SEGMENT_ADDRESS(segment_id)    (TUPLE_FOLD_LEFT_1(EVAL(segment_id), _MMAPWBS, PLUS) EVAL(WARMBOOT_RAM_SEGMENT_BASE))
#define WARMBOOT_GET_RAM_SEGMENT_SIZE(segment_id)       (TUPLE_ELEM_1(CAT(_MMAPWBS, EVAL(segment_id))))
#define WARMBOOT_GET_RAM_SEGMENT_ATTRIBS(segment_id)    (TUPLE_ELEM_2(CAT(_MMAPWBS, EVAL(segment_id))))

#define TZRAM_GET_SEGMENT_PA(segment_id)            (0x7C010000ull + (TUPLE_ELEM_0(CAT(_MMAPTZS, EVAL(segment_id)))))
#define TZRAM_GET_SEGMENT_5X_PA(segment_id)         (0x7C010000ull + (TUPLE_ELEM_0(CAT(_MMAPTZ5XS, EVAL(segment_id)))))
#define TZRAM_GET_SEGMENT_ADDRESS(segment_id)       (TUPLE_FOLD_LEFT_2(EVAL(segment_id), _MMAPTZS, PLUS) EVAL(TZRAM_SEGMENT_BASE))
#define TZRAM_GET_SEGMENT_SIZE(segment_id)          (TUPLE_ELEM_1(CAT(_MMAPTZS, EVAL(segment_id))))
#define TZRAM_IS_SEGMENT_EXECUTABLE(segment_id)     (TUPLE_ELEM_3(CAT(_MMAPTZS, EVAL(segment_id))))

/**********************************************************************************************/
/* We don't need unmapping functions */

ALINLINE static inline void identity_map_mapping(uintptr_t *mmu_l1_tbl, uintptr_t *mmu_l3_tbl, uintptr_t addr, size_t size, uint64_t attribs, bool is_block_range) {
    if (is_block_range) {
        mmu_map_block_range(1, mmu_l1_tbl, addr, addr, size, attribs | MMU_PTE_BLOCK_INNER_SHAREBLE | ATTRIB_MEMTYPE_NORMAL);
    }
    else {
        mmu_map_page_range(mmu_l3_tbl, addr, addr, size, attribs | MMU_PTE_BLOCK_INNER_SHAREBLE | ATTRIB_MEMTYPE_NORMAL);
    }
}
ALINLINE static inline void mmio_map_device(uintptr_t *mmu_l3_tbl, uintptr_t addr, uintptr_t pa, size_t size, bool is_secure) {
    static const uint64_t secure_device_attributes  = MMU_PTE_BLOCK_XN | MMU_PTE_BLOCK_INNER_SHAREBLE | ATTRIB_MEMTYPE_DEVICE;
    static const uint64_t device_attributes         = MMU_PTE_BLOCK_NS | secure_device_attributes;

    uint64_t attributes = is_secure ? secure_device_attributes : device_attributes;
    mmu_map_page_range(mmu_l3_tbl, addr, pa, size, attributes);
}

ALINLINE static inline void lp0_entry_map_ram_segment(uintptr_t *mmu_l3_tbl, uintptr_t addr, uintptr_t pa, size_t size, uint64_t attribs) {
    uint64_t attributes = MMU_PTE_BLOCK_XN | MMU_PTE_BLOCK_INNER_SHAREBLE | attribs;

    mmu_map_page_range(mmu_l3_tbl, addr, pa, size, attributes);
}

ALINLINE static inline void warmboot_map_ram_segment(uintptr_t *mmu_l3_tbl, uintptr_t addr, uintptr_t pa, size_t size, uint64_t attribs) {
    uint64_t attributes = MMU_PTE_BLOCK_XN | MMU_PTE_BLOCK_INNER_SHAREBLE | attribs;

    mmu_map_page_range(mmu_l3_tbl, addr, pa, size, attributes);
}

ALINLINE static inline void tzram_map_segment(uintptr_t *mmu_l3_tbl, uintptr_t addr, uintptr_t pa, size_t size, bool is_executable) {
    uint64_t attributes = (is_executable ? 0 : MMU_PTE_BLOCK_XN) | MMU_PTE_BLOCK_INNER_SHAREBLE | ATTRIB_MEMTYPE_NORMAL;

    mmu_map_page_range(mmu_l3_tbl, addr, pa, size, attributes);
}

#endif
