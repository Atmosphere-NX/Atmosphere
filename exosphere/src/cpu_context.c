#include <stdbool.h>
#include <stdint.h>
#include "arm.h"
#include "cpu_context.h"
#include "flow.h"
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


void power_down_current_core(void) {
    unsigned int current_core = get_core_id();
    flow_set_csr(current_core, 0);
    flow_set_halt_events(current_core, 0);
    flow_set_cc4_ctrl(current_core, 0);
    save_current_core_context();
    g_cpu_contexts[current_core].is_active = 0;
    flush_dcache_all();
    /* TODO: wait_for_power_off(), which writes to regs + . */
}

uint32_t cpu_off(void) {
    if (get_core_id() == 3) {
        power_down_current_core();
    } else {
         /* TODO: call_with_stack_pointer(get_powerdown_stack(), power_down_current_core); */
    }
    while (true) {
        /* Wait forever. */
    }
    return 0;
}

void save_current_core_context(void) {
    unsigned int current_core = get_core_id();
    uint64_t temp_reg = 1;
    /* Write 1 to OS lock .*/
    __asm__ __volatile__ ("msr oslar_el1, %0" : : "r"(temp_reg));
    
    /* Save system registers. */
    __asm__ __volatile__ ("mrs %0, osdtrrx_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].OSDTRRX_EL1 = (uint32_t)temp_reg;
    __asm__ __volatile__ ("mrs %0, mdscr_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].MDSCR_EL1 = (uint32_t)temp_reg;
    __asm__ __volatile__ ("mrs %0, oseccr_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].OSECCR_EL1 = (uint32_t)temp_reg;
    __asm__ __volatile__ ("mrs %0, mdccint_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].MDCCINT_EL1 = (uint32_t)temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgclaimclr_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGCLAIMCLR_EL1 = (uint32_t)temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgvcr32_el2" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGVCR32_EL2 = (uint32_t)temp_reg;
    __asm__ __volatile__ ("mrs %0, sder32_el3" : "=r"(temp_reg));
    g_cpu_contexts[current_core].SDER32_EL3 = (uint32_t)temp_reg;
    __asm__ __volatile__ ("mrs %0, mdcr_el2" : "=r"(temp_reg));
    g_cpu_contexts[current_core].MDCR_EL2 = (uint32_t)temp_reg;
    __asm__ __volatile__ ("mrs %0, mdcr_el3" : "=r"(temp_reg));
    g_cpu_contexts[current_core].MDCR_EL3 = (uint32_t)temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgbvr0_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGBVR0_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgbcr0_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGBCR0_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgbvr1_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGBVR1_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgbcr1_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGBCR1_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgbvr2_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGBVR2_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgbcr2_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGBCR2_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgbvr3_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGBVR3_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgbcr3_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGBCR3_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgbvr4_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGBVR4_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgbcr4_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGBCR4_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgbvr5_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGBVR5_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgbcr5_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGBCR5_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgwvr0_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGWVR0_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgwcr0_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGWCR0_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgwvr1_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGWVR1_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgwcr1_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGWCR1_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgwvr2_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGWVR2_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgwcr2_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGWCR2_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgwvr3_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGWVR3_EL1 = temp_reg;
    __asm__ __volatile__ ("mrs %0, dbgwcr3_el1" : "=r"(temp_reg));
    g_cpu_contexts[current_core].DBGWCR3_EL1 = temp_reg;

    /* Mark context as saved. */
    g_cpu_contexts[current_core].is_saved = 1;
}

void restore_current_core_context(void) {
    unsigned int current_core = get_core_id();
    uint64_t temp_reg;
    
    if (g_cpu_contexts[current_core].is_saved) {
        temp_reg = g_cpu_contexts[current_core].OSDTRRX_EL1;
        __asm__ __volatile__ ("msr osdtrrx_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].MDSCR_EL1;
        __asm__ __volatile__ ("msr mdscr_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].OSECCR_EL1;
        __asm__ __volatile__ ("msr oseccr_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].MDCCINT_EL1;
        __asm__ __volatile__ ("msr mdccint_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGCLAIMCLR_EL1;
        __asm__ __volatile__ ("msr dbgclaimclr_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGVCR32_EL2;
        __asm__ __volatile__ ("msr dbgvcr32_el2, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].SDER32_EL3;
        __asm__ __volatile__ ("msr sder32_el3, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].MDCR_EL2;
        __asm__ __volatile__ ("msr mdcr_el2, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].MDCR_EL3;
        __asm__ __volatile__ ("msr mdcr_el3, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGBVR0_EL1;
        __asm__ __volatile__ ("msr dbgbvr0_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGBCR0_EL1;
        __asm__ __volatile__ ("msr dbgbcr0_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGBVR1_EL1;
        __asm__ __volatile__ ("msr dbgbvr1_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGBCR1_EL1;
        __asm__ __volatile__ ("msr dbgbcr1_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGBVR2_EL1;
        __asm__ __volatile__ ("msr dbgbvr2_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGBCR2_EL1;
        __asm__ __volatile__ ("msr dbgbcr2_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGBVR3_EL1;
        __asm__ __volatile__ ("msr dbgbvr3_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGBCR3_EL1;
        __asm__ __volatile__ ("msr dbgbcr3_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGBVR4_EL1;
        __asm__ __volatile__ ("msr dbgbvr4_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGBCR4_EL1;
        __asm__ __volatile__ ("msr dbgbcr4_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGBVR5_EL1;
        __asm__ __volatile__ ("msr dbgbvr5_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGBCR5_EL1;
        __asm__ __volatile__ ("msr dbgbcr5_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGWVR0_EL1;
        __asm__ __volatile__ ("msr dbgwvr0_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGWCR0_EL1;
        __asm__ __volatile__ ("msr dbgwcr0_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGWVR1_EL1;
        __asm__ __volatile__ ("msr dbgwvr1_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGWCR1_EL1;
        __asm__ __volatile__ ("msr dbgwcr1_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGWVR2_EL1;
        __asm__ __volatile__ ("msr dbgwvr2_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGWCR2_EL1;
        __asm__ __volatile__ ("msr dbgwcr2_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGWVR3_EL1;
        __asm__ __volatile__ ("msr dbgwvr3_el1, %0" : : "r"(temp_reg));
        temp_reg = g_cpu_contexts[current_core].DBGWCR3_EL1;
        __asm__ __volatile__ ("msr dbgwcr3_el1, %0" : : "r"(temp_reg));
        
        g_cpu_contexts[current_core].is_saved = 0;
    }
}
