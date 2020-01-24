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
#include "utils.h"
#include "arm.h"
#include "mmu.h"
#include "memory_map.h"
#include "arm.h"
#include "package2.h"
#include "timers.h"
#include "exocfg.h"

#undef  MAILBOX_NX_BOOTLOADER_BASE
#undef  TIMERS_BASE
#define MAILBOX_NX_BOOTLOADER_BASE(targetfw)  (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_700) ? (MMIO_GET_DEVICE_7X_PA(MMIO_DEVID_NXBOOTLOADER_MAILBOX)) : (MMIO_GET_DEVICE_PA(MMIO_DEVID_NXBOOTLOADER_MAILBOX))
#define TIMERS_BASE                 (MMIO_GET_DEVICE_PA(MMIO_DEVID_TMRs_WDTs))

extern const uint8_t __start_cold[];

/* warboot_init.c */
extern unsigned int g_exosphere_target_firmware_for_init;
void init_dma_controllers(unsigned int target_firmware);
void set_memory_registers_enable_mmu_1x_ttbr0(void);
void set_memory_registers_enable_mmu_5x_ttbr0(void);

static void identity_map_all_mappings(uintptr_t *mmu_l1_tbl, uintptr_t *mmu_l3_tbl) {
    static const uintptr_t addrs[]      =   { TUPLE_FOLD_LEFT_0(EVAL(IDENTIY_MAPPING_ID_MAX), _MMAPID, COMMA) };
    static const size_t sizes[]         =   { TUPLE_FOLD_LEFT_1(EVAL(IDENTIY_MAPPING_ID_MAX), _MMAPID, COMMA) };
    static const uint64_t attribs[]     =   { TUPLE_FOLD_LEFT_2(EVAL(IDENTIY_MAPPING_ID_MAX), _MMAPID, COMMA) };
    static const uint64_t is_block[]    =   { TUPLE_FOLD_LEFT_3(EVAL(IDENTIY_MAPPING_ID_MAX), _MMAPID, COMMA) };

    for(size_t i = 0; i < IDENTIY_MAPPING_ID_MAX; i++) {
        identity_map_mapping(mmu_l1_tbl, mmu_l3_tbl, addrs[i], sizes[i], attribs[i], is_block[i]);
    }
}

static void mmio_map_all_devices(uintptr_t *mmu_l3_tbl, unsigned int target_firmware) {
    static const uintptr_t pas[]        =   { TUPLE_FOLD_LEFT_0(EVAL(MMIO_DEVID_MAX), _MMAPDEV, COMMA) };
    static const size_t sizes[]         =   { TUPLE_FOLD_LEFT_1(EVAL(MMIO_DEVID_MAX), _MMAPDEV, COMMA) };
    static const bool is_secure[]       =   { TUPLE_FOLD_LEFT_2(EVAL(MMIO_DEVID_MAX), _MMAPDEV, COMMA) };
    
    static const uintptr_t pas_7x[]     =   { TUPLE_FOLD_LEFT_0(EVAL(MMIO_DEVID_MAX), _MMAPDEV7X, COMMA) };

    for(size_t i = 0, offset = 0; i < MMIO_DEVID_MAX; i++) {
        uintptr_t pa = (target_firmware < ATMOSPHERE_TARGET_FIRMWARE_700) ? pas[i] : pas_7x[i];
        mmio_map_device(mmu_l3_tbl, MMIO_BASE + offset, pa, sizes[i], is_secure[i]);
        offset += sizes[i];
        offset += 0x1000;
    }
}

static void lp0_entry_map_all_ram_segments(uintptr_t *mmu_l3_tbl) {
    static const uintptr_t pas[]        =   { TUPLE_FOLD_LEFT_0(EVAL(LP0_ENTRY_RAM_SEGMENT_ID_MAX), _MMAPLP0ES, COMMA) };
    static const size_t sizes[]         =   { TUPLE_FOLD_LEFT_1(EVAL(LP0_ENTRY_RAM_SEGMENT_ID_MAX), _MMAPLP0ES, COMMA) };
    static const uint64_t attribs[]     =   { TUPLE_FOLD_LEFT_2(EVAL(LP0_ENTRY_RAM_SEGMENT_ID_MAX), _MMAPLP0ES, COMMA) };

    for(size_t i = 0, offset = 0; i < LP0_ENTRY_RAM_SEGMENT_ID_MAX; i++) {
        lp0_entry_map_ram_segment(mmu_l3_tbl, LP0_ENTRY_RAM_SEGMENT_BASE + offset, pas[i], sizes[i], attribs[i]);
        offset += 0x10000;
    }
}

static void warmboot_map_all_ram_segments(uintptr_t *mmu_l3_tbl) {
    static const uintptr_t pas[]        =   { TUPLE_FOLD_LEFT_0(EVAL(WARMBOOT_RAM_SEGMENT_ID_MAX), _MMAPWBS, COMMA) };
    static const size_t sizes[]         =   { TUPLE_FOLD_LEFT_1(EVAL(WARMBOOT_RAM_SEGMENT_ID_MAX), _MMAPWBS, COMMA) };
    static const uint64_t attribs[]     =   { TUPLE_FOLD_LEFT_2(EVAL(WARMBOOT_RAM_SEGMENT_ID_MAX), _MMAPWBS, COMMA) };

    for(size_t i = 0, offset = 0; i < WARMBOOT_RAM_SEGMENT_ID_MAX; i++) {
        warmboot_map_ram_segment(mmu_l3_tbl, WARMBOOT_RAM_SEGMENT_BASE + offset, pas[i], sizes[i], attribs[i]);
        offset += sizes[i];
    }
}

static void tzram_map_all_segments(uintptr_t *mmu_l3_tbl, unsigned int target_firmware) {
    static const uintptr_t offs[]       =   { TUPLE_FOLD_LEFT_0(EVAL(TZRAM_SEGMENT_ID_MAX), _MMAPTZS, COMMA) };
    static const size_t sizes[]         =   { TUPLE_FOLD_LEFT_1(EVAL(TZRAM_SEGMENT_ID_MAX), _MMAPTZS, COMMA) };
    static const size_t increments[]    =   { TUPLE_FOLD_LEFT_2(EVAL(TZRAM_SEGMENT_ID_MAX), _MMAPTZS, COMMA) };
    static const bool is_executable[]   =   { TUPLE_FOLD_LEFT_3(EVAL(TZRAM_SEGMENT_ID_MAX), _MMAPTZS, COMMA) };

    static const uintptr_t offs_5x[]    =   { TUPLE_FOLD_LEFT_0(EVAL(TZRAM_SEGMENT_ID_MAX), _MMAPTZ5XS, COMMA) };
    
    for(size_t i = 0, offset = 0; i < TZRAM_SEGMENT_ID_MAX; i++) {
        uintptr_t off = (target_firmware < ATMOSPHERE_TARGET_FIRMWARE_500) ? offs[i] : offs_5x[i];
        tzram_map_segment(mmu_l3_tbl, TZRAM_SEGMENT_BASE + offset, 0x7C010000ull + off, sizes[i], is_executable[i]);
        offset += increments[i];
    }
}

static void configure_ttbls(unsigned int target_firmware) {
    uintptr_t *mmu_l1_tbl;
    uintptr_t *mmu_l2_tbl; 
    uintptr_t *mmu_l3_tbl;
    if (target_firmware < ATMOSPHERE_TARGET_FIRMWARE_500) {
        mmu_l1_tbl = (uintptr_t *)(TZRAM_GET_SEGMENT_PA(TZRAM_SEGEMENT_ID_SECMON_EVT) + 0x800 - 64);
        mmu_l2_tbl = (uintptr_t *)TZRAM_GET_SEGMENT_PA(TZRAM_SEGMENT_ID_L2_TRANSLATION_TABLE);
        mmu_l3_tbl = (uintptr_t *)TZRAM_GET_SEGMENT_PA(TZRAM_SEGMENT_ID_L3_TRANSLATION_TABLE);
    } else {
        mmu_l1_tbl = (uintptr_t *)(TZRAM_GET_SEGMENT_5X_PA(TZRAM_SEGEMENT_ID_SECMON_EVT) + 0x800 - 64);
        mmu_l2_tbl = (uintptr_t *)TZRAM_GET_SEGMENT_5X_PA(TZRAM_SEGMENT_ID_L2_TRANSLATION_TABLE);
        mmu_l3_tbl = (uintptr_t *)TZRAM_GET_SEGMENT_5X_PA(TZRAM_SEGMENT_ID_L3_TRANSLATION_TABLE);
    }

    mmu_init_table(mmu_l1_tbl, 64); /* 33-bit address space */
    mmu_init_table(mmu_l2_tbl, 4096);
    /*
        Nintendo uses the same L3 table for everything, but they make sure
        nothing clashes.
    */
    mmu_init_table(mmu_l3_tbl, 4096);

    mmu_map_table(1, mmu_l1_tbl, 0x40000000, mmu_l2_tbl, 0);
    mmu_map_table(1, mmu_l1_tbl, 0x1C0000000, mmu_l2_tbl, 0);

    mmu_map_table(2, mmu_l2_tbl, 0x40000000, mmu_l3_tbl, 0);
    mmu_map_table(2, mmu_l2_tbl, 0x7C000000, mmu_l3_tbl, 0);
    mmu_map_table(2, mmu_l2_tbl, 0x1F0000000ull, mmu_l3_tbl, 0);

    identity_map_all_mappings(mmu_l1_tbl, mmu_l3_tbl);
    mmio_map_all_devices(mmu_l3_tbl, target_firmware);
    lp0_entry_map_all_ram_segments(mmu_l3_tbl);
    warmboot_map_all_ram_segments(mmu_l3_tbl);
    tzram_map_all_segments(mmu_l3_tbl, target_firmware);
}

static void do_relocation(const coldboot_crt0_reloc_list_t *reloc_list, size_t index) {
    extern const uint8_t __glob_origin__[];
    uint64_t *p_vma = (uint64_t *)reloc_list->relocs[index].vma;
    bool is_clear = reloc_list->relocs[index].lma == 0;
    size_t offset = reloc_list->relocs[index].lma - (uintptr_t)__glob_origin__;
    const uint64_t *p_lma = (const uint64_t *)(reloc_list->reloc_base + offset);
    size_t size = reloc_list->relocs[index].end_vma - reloc_list->relocs[index].vma;

    for(size_t i = 0; i < size / 8; i++) {
        p_vma[i] = is_clear ? 0 : p_lma[i];
    }
}

uintptr_t get_coldboot_crt0_temp_stack_address(void) {
    return TZRAM_GET_SEGMENT_PA(TZRAM_SEGMENT_ID_CORE3_STACK) + 0x800;
}

uintptr_t get_coldboot_crt0_stack_address(void) {
    if (exosphere_get_target_firmware_for_init() < ATMOSPHERE_TARGET_FIRMWARE_500) {
        return TZRAM_GET_SEGMENT_PA(TZRAM_SEGMENT_ID_CORE3_STACK) + 0x800;
    } else {
        return TZRAM_GET_SEGMENT_5X_PA(TZRAM_SEGMENT_ID_CORE3_STACK) + 0x800;
    }
    
}

void coldboot_init(coldboot_crt0_reloc_list_t *reloc_list, uintptr_t start_cold) {
    //MAILBOX_NX_SECMON_BOOT_TIME = TIMERUS_CNTR_1US_0;

    /* Custom approach */
    reloc_list->reloc_base = start_cold;

    /* TODO: Set NX BOOTLOADER clock time field */

    /* This at least copies .warm_crt0 to its VMA. */
    for(size_t i = 0; i < reloc_list->nb_relocs_pre_mmu_init; i++) {
        do_relocation(reloc_list, i);
    }
    /* At this point, we can (and will) access functions located in .warm_crt0 */

    /*
        From https://events.static.linuxfound.org/sites/events/files/slides/slides_17.pdf :
        Caches may write back dirty lines at any time:
            - To make space for new allocations
            - Even if MMU is off
            - Even if Cacheable accesses are disabled (caches are never 'off')

        It should be fine to clear that here and not before.
    */
    flush_dcache_all();
    invalidate_icache_all();

    /* Set target firmware. */
    g_exosphere_target_firmware_for_init = exosphere_get_target_firmware_for_init();

    /* Initialize DMA controllers, and write to AHB_GIZMO_TZRAM. */
    /* TZRAM accesses should work normally after this point. */
    init_dma_controllers(g_exosphere_target_firmware_for_init);

    configure_ttbls(g_exosphere_target_firmware_for_init);
    if (g_exosphere_target_firmware_for_init < ATMOSPHERE_TARGET_FIRMWARE_500) {
        set_memory_registers_enable_mmu_1x_ttbr0();
    } else {
        set_memory_registers_enable_mmu_5x_ttbr0();
    }

    /* Copy or clear the remaining sections */
    for(size_t i = 0; i < reloc_list->nb_relocs_post_mmu_init; i++) {
        do_relocation(reloc_list, reloc_list->nb_relocs_pre_mmu_init + i);
    }

    flush_dcache_all();
    invalidate_icache_all();
    /* At this point we can access all the mapped segments (all other functions, data...) normally */
}
