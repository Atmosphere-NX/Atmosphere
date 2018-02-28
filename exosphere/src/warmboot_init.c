#include "utils.h"
#include "memory_map.h"

void __set_memory_registers(uintptr_t ttbr0, uintptr_t vbar, uint64_t cpuectlr, uint32_t scr,
                            uint32_t tcr, uint32_t cptr, uint64_t mair, uint32_t sctlr);

uintptr_t get_warmboot_crt0_stack_address(void);
void set_memory_registers(void);

void flush_dcache_all_tzram_pa(void) {
    /* TODO */
}

void invalidate_icache_all_tzram_pa(void) {
    /* TODO */
}

uintptr_t get_warmboot_crt0_stack_address(void) {
    return TZRAM_GET_SEGMENT_PA(TZRAM_SEGMENT_ID_CORE012_STACK) + 0x800;
}

void warmboot_init(void) {
    /* TODO: Implement. */
}
