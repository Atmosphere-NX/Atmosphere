#include "utils.h"
#include "mmu.h"
#include "memory_map.h"

extern void (*__fini_array_start[])(void);
extern void (*__fini_array_end[])(void);
extern void _fini(void);

extern void __jump_to_lower_el(uint64_t arg, uintptr_t ep, unsigned int el);

void coldboot_main(void);

/* Needs to be called for EL3->EL3 chainloading (and only in that case). TODO: use it */
static void __libc_fini_array(void) __attribute__((used)) {
    for (size_t i = __fini_array_end - __fini_array_start; i > 0; i--)
        __fini_array_start[i - 1]();
    _fini(); /* FIXME: do we have this gcc-provided symbol if we build with -nostartfiles? */
}

void coldboot_main(void) {
    /* TODO */
}
