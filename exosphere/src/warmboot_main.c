#include "utils.h"
#include "mmu.h"
#include "memory_map.h"
#include "cpu_context.h"

void warmboot_main(void) {
    /* TODO: lots of stuff */
    core_jump_to_lower_el();
}
