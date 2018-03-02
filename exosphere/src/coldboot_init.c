#include <string.h>
#include "utils.h"
#include "mmu.h"
#include "memory_map.h"
#include "arm.h"

extern const uint8_t __start_cold[];

/* warmboot_init.c */
void set_memory_registers_enable_mmu(void);

static void identity_map_all_mappings(uintptr_t *mmu_l1_tbl, uintptr_t *mmu_l3_tbl) {
    static const uintptr_t addrs[]      =   { TUPLE_FOLD_LEFT_0(EVAL(IDENTIY_MAPPING_ID_MAX), _MMAPID, COMMA) };
    static const size_t sizes[]         =   { TUPLE_FOLD_LEFT_1(EVAL(IDENTIY_MAPPING_ID_MAX), _MMAPID, COMMA) };
    static const uint64_t attribs[]     =   { TUPLE_FOLD_LEFT_2(EVAL(IDENTIY_MAPPING_ID_MAX), _MMAPID, COMMA) };
    static const uint64_t is_block[]    =   { TUPLE_FOLD_LEFT_3(EVAL(IDENTIY_MAPPING_ID_MAX), _MMAPID, COMMA) };

    for(size_t i = 0; i < IDENTIY_MAPPING_ID_MAX; i++) {
        identity_map_mapping(mmu_l1_tbl, mmu_l3_tbl, addrs[i], sizes[i], attribs[i], is_block[i]);
    }
}

static void mmio_map_all_devices(uintptr_t *mmu_l3_tbl) {
    static const uintptr_t pas[]        =   { TUPLE_FOLD_LEFT_0(EVAL(MMIO_DEVID_MAX), _MMAPDEV, COMMA) };
    static const size_t sizes[]         =   { TUPLE_FOLD_LEFT_1(EVAL(MMIO_DEVID_MAX), _MMAPDEV, COMMA) };
    static const bool is_secure[]       =   { TUPLE_FOLD_LEFT_2(EVAL(MMIO_DEVID_MAX), _MMAPDEV, COMMA) };

    for(size_t i = 0, offset = 0; i < MMIO_DEVID_MAX; i++) {
        mmio_map_device(mmu_l3_tbl, MMIO_BASE + offset, pas[i], sizes[i], is_secure[i]);
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

static void tzram_map_all_segments(uintptr_t *mmu_l3_tbl) {
    static const uintptr_t offs[]       =   { TUPLE_FOLD_LEFT_0(EVAL(TZRAM_SEGMENT_ID_MAX), _MMAPTZS, COMMA) };
    static const size_t sizes[]         =   { TUPLE_FOLD_LEFT_1(EVAL(TZRAM_SEGMENT_ID_MAX), _MMAPTZS, COMMA) };
    static const size_t increments[]    =   { TUPLE_FOLD_LEFT_2(EVAL(TZRAM_SEGMENT_ID_MAX), _MMAPTZS, COMMA) };
    static const bool is_executable[]   =   { TUPLE_FOLD_LEFT_3(EVAL(TZRAM_SEGMENT_ID_MAX), _MMAPTZS, COMMA) };

    for(size_t i = 0, offset = 0; i < TZRAM_SEGMENT_ID_MAX; i++) {
        tzram_map_segment(mmu_l3_tbl, TZRAM_SEGMENT_BASE + offset, 0x7C010000ull + offs[i], sizes[i], is_executable[i]);
        offset += increments[i];
    }
}

static void configure_ttbls(void) {
    uintptr_t *mmu_l1_tbl = (uintptr_t *)(TZRAM_GET_SEGMENT_PA(TZRAM_SEGEMENT_ID_SECMON_EVT) + 0x800 - 64);
    uintptr_t *mmu_l2_tbl = (uintptr_t *)TZRAM_GET_SEGMENT_PA(TZRAM_SEGMENT_ID_L2_TRANSLATION_TABLE);
    uintptr_t *mmu_l3_tbl = (uintptr_t *)TZRAM_GET_SEGMENT_PA(TZRAM_SEGMENT_ID_L3_TRANSLATION_TABLE);

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
    mmio_map_all_devices(mmu_l3_tbl);
    lp0_entry_map_all_ram_segments(mmu_l3_tbl);
    warmboot_map_all_ram_segments(mmu_l3_tbl);
    tzram_map_all_segments(mmu_l3_tbl);
}

static void translate_warmboot_func_list(coldboot_crt0_reloc_list_t *reloc_list) {
    coldboot_crt0_reloc_t *warmboot_crt0_reloc = &reloc_list->relocs[0];
    coldboot_crt0_reloc_t *main_reloc = &reloc_list->relocs[reloc_list->nb_relocs_pre_mmu_init];

    /* The main segment immediately follows the warmboot crt0 in TZRAM, in the same page. */
    uintptr_t main_pa = (uintptr_t)warmboot_crt0_reloc->vma | ((uintptr_t)main_reloc->vma & ~0xFFF);
    for(size_t i = 0; i < reloc_list->func_list->nb_funcs; i++) {
        if(reloc_list->func_list->addrs[i] >= 0x1F0000000ull) {
            reloc_list->func_list->addrs[i] = main_pa + reloc_list->func_list->addrs[i] - (uintptr_t)main_reloc->vma;
        }
    }
}

static void do_relocation(const coldboot_crt0_reloc_list_t *reloc_list, size_t index) {
    uint64_t *p_vma = (uint64_t *)reloc_list->relocs[index].vma;
    const uint64_t *p_lma = (const uint64_t *)(reloc_list->reloc_base + reloc_list->relocs[index].reloc_offset);
    size_t size = reloc_list->relocs[index].end_vma - reloc_list->relocs[index].vma;

    for(size_t i = 0; i < size / 8; i++) {
        p_vma[i] = reloc_list->relocs[index].reloc_offset != 0 ? p_lma[i] : 0;
    }
}

uintptr_t get_coldboot_crt0_stack_address(void) {
    return TZRAM_GET_SEGMENT_PA(TZRAM_SEGMENT_ID_CORE3_STACK) + 0x800;
}

void coldboot_init(coldboot_crt0_reloc_list_t *reloc_list) {
    /* Custom approach */
    reloc_list->reloc_base = (uintptr_t)__start_cold;

    /* TODO: Set NX BOOTLOADER clock time field */

    /* This at least copies .warm_crt0 to its VMA. */
    for(size_t i = 0; i < reloc_list->nb_relocs_pre_mmu_init; i++) {
        do_relocation(reloc_list, i);
    }
    /* At this point, we can (and will) access functions located in .warm_crt0 */

    translate_warmboot_func_list(reloc_list);

    /* TODO: initialize DMA controllers, etc. */
    configure_ttbls();
    reloc_list->func_list->funcs.set_memory_registers_enable_mmu();

    /* Copy or clear the remaining sections */
    for(size_t i = 0; i < reloc_list->nb_relocs_post_mmu_init; i++) {
        do_relocation(reloc_list, reloc_list->nb_relocs_pre_mmu_init + i);
    }

    reloc_list->func_list->funcs.flush_dcache_all();
    reloc_list->func_list->funcs.invalidate_icache_all();
    /* At this point we can access all the mapped segments (all other functions, data...) normally */
}
