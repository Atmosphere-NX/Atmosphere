#include <stdint.h>
#include "cpu_context.h"
#include "pmc.h"
#include "timers.h"
#include "utils.h"

static saved_cpu_context_t g_cpu_contexts[NUM_CPU_CORES] = {0};

void set_core_entrypoint_and_argument(uint32_t core, uint64_t entrypoint_addr, uint64_t argument) {
    g_cpu_contexts[core].ELR_EL3 = entrypoint_addr;
    g_cpu_contexts[core].argument = argument;
}

uint32_t cpu_on(uint32_t core, uint64_t entrypoint_addr, uint64_t argument) {
    /* Is core valid? */
    if (core >= NUM_CPU_CORES) {
        return 0xFFFFFFFE;
    }
    
    /* Is core already on? */
    if (g_cpu_contexts[core].is_active) {
        return 0xFFFFFFFC;
    }
    
    set_core_entrypoint_and_argument(core, entrypoint_addr, argument);
    
    const uint32_t status_masks[NUM_CPU_CORES] = {0x4000, 0x200, 0x400, 0x800};
    const uint32_t toggle_vals[NUM_CPU_CORES] = {0xE, 0x9, 0xA, 0xB};
    
    /* Check if we're already in the correct state. */
    if ((APBDEV_PMC_PWRGATE_STATUS_0 & status_masks[core]) != status_masks[core]) {
        uint32_t counter = 5001;
        
        /* Poll the start bit until 0 */
        while (APBDEV_PMC_PWRGATE_TOGGLE_0 & 0x100) {
            wait(1);
            counter--;
            if (counter < 1) {
                return 0;
            }
        }
        
        /* Program PWRGATE_TOGGLE with the START bit set to 1, selecting CE[N] */
        APBDEV_PMC_PWRGATE_TOGGLE_0 = toggle_vals[core] | 0x100;
        
        /* Poll until we're in the correct state. */
        counter = 5001;
        while (counter > 0) {
            if ((APBDEV_PMC_PWRGATE_STATUS_0 & status_masks[core]) == status_masks[core]) {
                break;
            }
            wait(1);
            counter--;
        }
    }
    return 0;
}

uint32_t cpu_off(void) {
    return 0;
    /* TODO */
}
