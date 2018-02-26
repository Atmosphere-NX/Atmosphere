#include "utils.h"
#include "memory_map.h"

uintptr_t get_warmboot_crt0_stack_address(void);

void flush_dcache_all_tzram_pa(void) {
    /* TODO */
}

void invalidate_icache_all_tzram_pa(void) {
    /* TODO */
}

uintptr_t get_warmboot_crt0_stack_address(void) {
    return TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGMENT_ID_CORE3_STACK) + 0x800;
}
