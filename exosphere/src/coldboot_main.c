#include <string.h>
#include "utils.h"
#include "mmu.h"
#include "memory_map.h"
#include "cache.h"

/*
extern void (*__fini_array_start[])(void);
extern void (*__fini_array_end[])(void);
extern void _fini(void);*/

extern uint8_t __pk2ldr_start__[], __pk2ldr_end__[];

extern void __jump_to_lower_el(uint64_t arg, uintptr_t ep, unsigned int el);

void coldboot_main(void);

#if 0
/* Needs to be called for EL3->EL3 chainloading (and only in that case). TODO: use it */
__attribute__((used)) static void __libc_fini_array(void) {
    for (size_t i = __fini_array_end - __fini_array_start; i > 0; i--)
        __fini_array_start[i - 1]();
    _fini(); /* FIXME: do we have this gcc-provided symbol if we build with -nostartfiles? */
}
#endif

void coldboot_main(void) {
    #if 0
    uintptr_t *mmu_l3_table = (uintptr_t *)TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGMENT_ID_L3_TRANSLATION_TABLE);
    uintptr_t pk2ldr = TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGMENT_ID_PK2LDR);

    /* Clear and unmap pk2ldr (which is reused as exception entry stacks) */
    memset((void *)pk2ldr, 0, __pk2ldr_end__ - __pk2ldr_start__);
    mmu_unmap_range(3, mmu_l3_table, pk2ldr, __pk2ldr_end__ - __pk2ldr_start__);
    tlb_invalidate_all_inner_shareable();
    #endif

    /* TODO: stuff & jump to lower EL */
}
