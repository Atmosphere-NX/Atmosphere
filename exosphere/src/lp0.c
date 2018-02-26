#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "utils.h"

#include "bpmp.h"
#include "configitem.h"
#include "flow.h"
#include "fuse.h"
#include "i2c.h"
#include "lp0.h"
#include "pmc.h"
#include "se.h"
#include "smc_api.h"
#include "timers.h"

extern const uint8_t bpmpfw_bin[];
extern const uint32_t bpmpfw_bin_size;

/* Save security engine, and go to sleep. */
void save_se_and_power_down_cpu(void) {
    clear_priv_smc_in_progress();
    /* TODO. */
}

uint32_t cpu_suspend(uint64_t power_state, uint64_t entrypoint, uint64_t argument) {
    /* Ensure SMC call is to enter deep sleep. */
    if ((power_state & 0x17FFF) != 0x1001B) {
        return 0xFFFFFFFD;
    }
    
    unsigned int current_core = get_core_id();
    
    /* TODO: Enable clock and reset for I2C1. */
    if (configitem_should_profile_battery() && !i2c_query_ti_charger_bit_7()) {
        /* Profile the battery. */
        i2c_set_ti_charger_bit_7();
        uint32_t start_time = get_time();
        bool should_wait = true;
        /* TODO: This is GPIO-6 GPIO_IN_1 */
        while ((*((volatile uint32_t *)(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_GPIO) + 0x634))) & 1) {
            if (get_time() - start_time > 50000) {
                should_wait = false;
                break;
            }
        }
        if (should_wait) {
            wait(0x100);
        }
    }
    /* TODO: Reset I2C1 controller. */
    
    /* Enable LP0 Wake Event Detection. */
    wait(75);
    APBDEV_PMC_CNTRL2_0 |= 0x200; /* Set WAKE_DET_EN. */
    wait(75);
    APBDEV_PM_0 = 0xFFFFFFFF; /* Set all wake events. */
    APBDEV_PMC_WAKE2_STATUS_0 = 0xFFFFFFFF; /* Set all wake events. */
    wait(75);
    
    /* TODO: Enable I2C5 Clock/Reset. */
    if (fuse_get_bootrom_patch_version() >= 0x7F) {
        i2c_send_pmic_cpu_shutdown_cmd();
    }
    
    /* Jamais Vu mitigation #1: Ensure all other cores are off. */
    if (APBDEV_PMC_PWRGATE_STATUS_0 & 0xE00) {
        generic_panic();
    }
    
    /* Jamais Vu mitigation #2: Ensure the BPMP is halted. */
    if ((get_debug_authentication_status() & 3) == 3) {
        /* BPMP should just be plainly halted, in debugging conditions. */
        if (FLOW_CTLR_HALT_COP_EVENTS_0 != 0x50000000) {
            generic_panic();
        }
    } else {
        /* BPMP must be in never-woken-up halt mode, under normal conditions. */
        if (FLOW_CTLR_HALT_COP_EVENTS_0 != 0x40000000) {
            generic_panic();
        }
    }
    
    /* TODO: Jamais Vu mitigation #3: Ensure all relevant DMA controllers are held in reset. */
    /* This just requires checking CLK_RST_CONTROLLER_RST_DEVICES_H_0 & mask == 0x4000004. */
    
    /* Signal to bootrom the next reset should be a warmboot. */
    APBDEV_PMC_SCRATCH0_0 = 1; 
    APBDEV_PMC_DPD_ENABLE_0 |= 2;
    
    /* Prepare to boot the BPMP running our deep sleep firmware. */
    
    /* Mark PMC registers as not secure-world only, so BPMP can access them. */
    (*((volatile uint32_t *)(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_MISC) + 0xC00))) &= 0xFFFFDFFF; /* TODO: macro */
    
    /* Setup BPMP vectors. */
    BPMP_VECTOR_RESET = 0x40003000; /* lp0_entry_firmware_crt0 */
    BPMP_VECTOR_UNDEF = 0x40003004; /* Reboot. */
    BPMP_VECTOR_SWI = 0x40003004; /* Reboot. */
    BPMP_VECTOR_PREFETCH_ABORT = 0x40003004; /* Reboot. */
    BPMP_VECTOR_DATA_ABORT = 0x40003004; /* Reboot. */
    BPMP_VECTOR_UNK = 0x40003004; /* Reboot. */
    BPMP_VECTOR_IRQ = 0x40003004; /* Reboot. */
    BPMP_VECTOR_FIQ = 0x40003004; /* Reboot. */
    
    /* TODO: Hold the BPMP in reset. */
    uint8_t *lp0_entry_code = (uint8_t *)(LP0_ENTRY_GET_RAM_SEGMENT_ADDRESS(LP0_ENTRY_RAM_SEGMENT_ID_LP0_ENTRY_CODE));
    (void)(lp0_entry_code);
    /* TODO: memcpy(lp0_entry_code, BPMP_FIRMWARE_ADDRESS, sizeof(BPMP_FIRMWARE)); */
    /* TODO: flush_dcache_range(lp0_entry_code, lp0_entry_code + sizeof(BPMP_FIRMWARE)); */
    /* TODO: Take the BPMP out of reset. */
    
    /* Start executing BPMP firmware. */
    FLOW_CTLR_HALT_COP_EVENTS_0 = 0;
    /* Prepare the current core for sleep. */
    flow_set_cc4_ctrl(current_core, 0);
    flow_set_halt_events(current_core, 0);
    FLOW_CTLR_L2FLUSH_CONTROL_0 = 0;
    flow_set_csr(current_core, 2);
    
    /* Save core context. */
    set_core_entrypoint_and_argument(current_core, entrypoint, argument);
    /* TODO: save_current_core_context(); */
    /* TODO: set_current_core_inacctive(); */
    /* TODO: set_current_core_saved(true); */
    /* TODO: call_with_stack_pointer(TZRAM_GET_SEGMENT_ADDR(TZRAM_SEGMENT_ID_CORE012_STACK) + 0x1000ULL, save_se_and_power_down_cpu); */
    
    /* NOTE: This return never actually occurs. */
    return 0;
}
