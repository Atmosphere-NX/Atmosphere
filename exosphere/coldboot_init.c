#include "utils.h"
#include "mmu.h"
#include "memory_map.h"

extern void (*__preinit_array_start[])(void);
extern void (*__preinit_array_end[])(void);
extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);
extern void _init(void);

extern uint8_t __warmboot_crt0_start__[], __warmboot_crt0_end__[], __warmboot_crt0_lma__[];
extern uint8_t __main_start__[], __main_end__[], __main_lma__[];
extern uint8_t __pk2ldr_start__[], __pk2ldr_end__[], __pk2ldr_lma__[];
extern uint8_t __vectors_start__[], __vectors_end__[], __vectors_lma__[];
extern void flush_dcache_all_tzram_pa(void);
extern void invalidate_icache_all_tzram_pa(void);

uintptr_t get_coldboot_crt0_stack_address(void);

static void configure_ttbls(void) {
    uintptr_t *mmu_l1_tbl = (uintptr_t *)(tzram_get_segment_pa(TZRAM_SEGEMENT_ID_SECMON_EVT) + 0x800 - 64);
    uintptr_t *mmu_l2_tbl = (uintptr_t *)tzram_get_segment_pa(TZRAM_SEGMENT_ID_L2_TRANSLATION_TABLE);
    uintptr_t *mmu_l3_tbl = (uintptr_t *)tzram_get_segment_pa(TZRAM_SEGMENT_ID_L3_TRANSLATION_TABLE);
    
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
    lp0_map_all_plaintext_ram_segments(mmu_l3_tbl);
    lp0_map_all_ciphertext_ram_segments(mmu_l3_tbl);
    tzram_map_all_segments(mmu_l3_tbl);
}

static void copy_lma_to_vma(unsigned int segment_id, void *lma, size_t size, bool vma_is_pa) {
    uintptr_t vma = vma_is_pa ? tzram_get_segment_pa(segment_id) : tzram_get_segment_address(segment_id);
    uintptr_t vma_offset = (uintptr_t)lma & 0xFFF;
    uint64_t *p_vma = (uint64_t *)vma;
    uint64_t *p_lma = (uint64_t *)lma;
    for (size_t i = 0; i < size / 8; i++) {
        p_vma[vma_offset / 8 + i] = p_lma[i]; 
    }
}

static void __libc_init_array(void) {
    for (size_t i = 0; i < __preinit_array_end - __preinit_array_start; i++)
        __preinit_array_start[i]();
    _init(); /* FIXME: do we have this gcc-provided symbol if we build with -nostartfiles? */
    for (size_t i = 0; i < __init_array_end - __init_array_start; i++)
        __init_array_start[i]();
}

uintptr_t get_coldboot_crt0_stack_address(void) {
    return tzram_get_segment_pa(TZRAM_SEGMENT_ID_CORE3_STACK) + 0x800;
}

void coldboot_init(void) {
    /* TODO: Set NX BOOTLOADER clock time field */
    copy_lma_to_vma(TZRAM_SEGMENT_ID_WARMBOOT_CRT0_AND_MAIN, __warmboot_crt0_lma__, __warmboot_crt0_end__ - __warmboot_crt0_start__, true);
    /* TODO: set some mmio regs, etc. */
    /* TODO: initialize DMA controllers */
    configure_ttbls();
    copy_lma_to_vma(TZRAM_SEGMENT_ID_WARMBOOT_CRT0_AND_MAIN, __main_lma__, __main_end__ - __main_start__, false);
    copy_lma_to_vma(TZRAM_SEGMENT_ID_PK2LDR, __pk2ldr_lma__, __pk2ldr_end__ - __pk2ldr_start__, false);
    copy_lma_to_vma(TZRAM_SEGEMENT_ID_SECMON_EVT, __vectors_lma__, __vectors_end__ - __vectors_start__, false);
    /* TODO: set the MMU regs & tlbi & enable MMU */

    flush_dcache_all_tzram_pa();
    invalidate_icache_all_tzram_pa();
    /* TODO: zero-initialize the cpu context */
    /* Nintendo clears the (emtpy) pk2ldr's BSS section, but we embed it 0-filled in the binary */
    __libc_init_array(); /* construct global objects */
}
