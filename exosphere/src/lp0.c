#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "utils.h"

#include "car.h"
#include "bpmp.h"
#include "arm.h"
#include "configitem.h"
#include "cpu_context.h"
#include "flow.h"
#include "fuse.h"
#include "i2c.h"
#include "lp0.h"
#include "masterkey.h"
#include "pmc.h"
#include "se.h"
#include "smc_api.h"
#include "timers.h"
#include "misc.h"
#include "exocfg.h"

#define u8 uint8_t
#define u32 uint32_t
#include "bpmpfw_bin.h"
#undef u8
#undef u32

/* Save security engine, and go to sleep. */
void save_se_and_power_down_cpu(void) {
    uint32_t tzram_cmac[0x4] = {0};

    uint8_t *tzram_encryption_dst = (uint8_t *)(LP0_ENTRY_GET_RAM_SEGMENT_ADDRESS(LP0_ENTRY_RAM_SEGMENT_ID_ENCRYPTED_TZRAM));
    uint8_t *tzram_encryption_src = (uint8_t *)(LP0_ENTRY_GET_RAM_SEGMENT_ADDRESS(LP0_ENTRY_RAM_SEGMENT_ID_CURRENT_TZRAM));
    uint8_t *tzram_store_address = (uint8_t *)(WARMBOOT_GET_RAM_SEGMENT_ADDRESS(WARMBOOT_RAM_SEGMENT_ID_TZRAM));
    clear_priv_smc_in_progress();

    /* Flush cache. */
    flush_dcache_all();

    /* Encrypt and save TZRAM into DRAM using a random aes-256 key. */
    se_generate_random_key(KEYSLOT_SWITCH_LP0TZRAMKEY, KEYSLOT_SWITCH_RNGKEY);

    flush_dcache_range(tzram_encryption_dst, tzram_encryption_dst + LP0_TZRAM_SAVE_SIZE);
    flush_dcache_range(tzram_encryption_src, tzram_encryption_src + LP0_TZRAM_SAVE_SIZE);

    /* Use the all-zero cmac buffer as an IV. */
    se_aes_256_cbc_encrypt(KEYSLOT_SWITCH_LP0TZRAMKEY, tzram_encryption_dst, LP0_TZRAM_SAVE_SIZE, tzram_encryption_src, LP0_TZRAM_SAVE_SIZE, tzram_cmac);
    flush_dcache_range(tzram_encryption_dst, tzram_encryption_dst + LP0_TZRAM_SAVE_SIZE);

    /* Copy encrypted TZRAM from IRAM to DRAM. */
    memcpy(tzram_store_address, tzram_encryption_dst, LP0_TZRAM_SAVE_SIZE);
    flush_dcache_range(tzram_store_address, tzram_store_address + LP0_TZRAM_SAVE_SIZE);

    /* Compute CMAC. */
    se_compute_aes_256_cmac(KEYSLOT_SWITCH_LP0TZRAMKEY, tzram_cmac, sizeof(tzram_cmac), tzram_encryption_src, LP0_TZRAM_SAVE_SIZE);

    /* Write CMAC, lock registers. */
    APBDEV_PMC_SECURE_SCRATCH112_0 = tzram_cmac[0];
    APBDEV_PMC_SECURE_SCRATCH113_0 = tzram_cmac[1];
    APBDEV_PMC_SECURE_SCRATCH114_0 = tzram_cmac[2];
    APBDEV_PMC_SECURE_SCRATCH115_0 = tzram_cmac[3];
    APBDEV_PMC_SEC_DISABLE8_0 = 0x550000;

    /* Save security engine state. */
    uint8_t *se_state_dst = (uint8_t *)(WARMBOOT_GET_RAM_SEGMENT_ADDRESS(WARMBOOT_RAM_SEGMENT_ID_SE_STATE));
    se_check_error_status_reg();
    se_set_in_context_save_mode(true);
    se_save_context(KEYSLOT_SWITCH_SRKGENKEY, KEYSLOT_SWITCH_RNGKEY, se_state_dst);
    flush_dcache_range(se_state_dst, se_state_dst + 0x840);
    APBDEV_PMC_SCRATCH43_0 = (uint32_t)(WARMBOOT_GET_RAM_SEGMENT_PA(WARMBOOT_RAM_SEGMENT_ID_SE_STATE));
    se_set_in_context_save_mode(false);
    se_check_error_status_reg();

    if (!configitem_is_retail()) {
        /* TODO: uart_log("OYASUMI"); */
    }

    finalize_powerdown();
}

uint32_t cpu_suspend(uint64_t power_state, uint64_t entrypoint, uint64_t argument) {
    /* Ensure SMC call is to enter deep sleep. */
    if ((power_state & 0x17FFF) != 0x1001B) {
        return 0xFFFFFFFD;
    }

    unsigned int current_core = get_core_id();

    clkrst_reboot(CARDEVICE_I2C1);

    if (configitem_should_profile_battery() && !i2c_query_ti_charger_bit_7()) {
        /* Profile the battery. */
        i2c_set_ti_charger_bit_7();
        uint32_t start_time = get_time();
        bool should_wait = true;
        /* TODO: This is GPIO-6 GPIO_IN_1 */
        while (MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_GPIO) + 0x634) & 1) {
            if (get_time() - start_time > 50000) {
                should_wait = false;
                break;
            }
        }
        if (should_wait) {
            wait(0x100);
        }
    }
    clkrst_disable(CARDEVICE_I2C1);

    /* Enable LP0 Wake Event Detection. */
    wait(75);
    APBDEV_PMC_CNTRL2_0 |= 0x200; /* Set WAKE_DET_EN. */
    wait(75);
    APBDEV_PM_0 = 0xFFFFFFFF; /* Set all wake events. */
    APBDEV_PMC_WAKE2_STATUS_0 = 0xFFFFFFFF; /* Set all wake events. */
    wait(75);

    clkrst_reboot(CARDEVICE_I2C5);
    if (fuse_get_bootrom_patch_version() >= 0x7F) {
        i2c_send_pmic_cpu_shutdown_cmd();
    }

    /* Jamais Vu mitigation #1: Ensure all other cores are off. */
    if (APBDEV_PMC_PWRGATE_STATUS_0 & 0xE00) {
        generic_panic();
    }

    /* For debugging, make this check always pass. */
    if ((exosphere_get_target_firmware() < EXOSPHERE_TARGET_FIRMWARE_400 || (get_debug_authentication_status() & 3) == 3)) {
        FLOW_CTLR_HALT_COP_EVENTS_0 = 0x50000000;
    } else {
        FLOW_CTLR_HALT_COP_EVENTS_0 = 0x40000000;
    }

    /* Jamais Vu mitigation #2: Ensure the BPMP is halted. */
    if (exosphere_get_target_firmware() < EXOSPHERE_TARGET_FIRMWARE_400 || (get_debug_authentication_status() & 3) == 3) {
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

    /* Jamais Vu mitigation #3: Ensure all relevant DMA controllers are held in reset. */
    if ((CLK_RST_CONTROLLER_RST_DEVICES_H_0 & 0x4000004) != 0x4000004) {
        generic_panic();
    }

    /* Signal to bootrom the next reset should be a warmboot. */
    APBDEV_PMC_SCRATCH0_0 = 1;
    APBDEV_PMC_DPD_ENABLE_0 |= 2;

    /* Prepare to boot the BPMP running our deep sleep firmware. */

    /* Mark PMC registers as not secure-world only, so BPMP can access them. */
    APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG0_0 &= 0xFFFFDFFF;

    /* Setup BPMP vectors. */
    BPMP_VECTOR_RESET = 0x40003000; /* lp0_entry_firmware_crt0 */
    BPMP_VECTOR_UNDEF = 0x40003004; /* Reboot. */
    BPMP_VECTOR_SWI = 0x40003004; /* Reboot. */
    BPMP_VECTOR_PREFETCH_ABORT = 0x40003004; /* Reboot. */
    BPMP_VECTOR_DATA_ABORT = 0x40003004; /* Reboot. */
    BPMP_VECTOR_UNK = 0x40003004; /* Reboot. */
    BPMP_VECTOR_IRQ = 0x40003004; /* Reboot. */
    BPMP_VECTOR_FIQ = 0x40003004; /* Reboot. */

    /* Hold the BPMP in reset. */
    MAKE_CAR_REG(0x300) = 2;

    /* Copy BPMP firmware. */
    uint8_t *lp0_entry_code = (uint8_t *)(LP0_ENTRY_GET_RAM_SEGMENT_ADDRESS(LP0_ENTRY_RAM_SEGMENT_ID_LP0_ENTRY_CODE));
    memcpy(lp0_entry_code, bpmpfw_bin, bpmpfw_bin_size);
    flush_dcache_range(lp0_entry_code, lp0_entry_code + bpmpfw_bin_size);

    /* Take the BPMP out of reset. */
    MAKE_CAR_REG(0x304) = 2;

    /* Start executing BPMP firmware. */
    FLOW_CTLR_HALT_COP_EVENTS_0 = 0;
    /* Prepare the current core for sleep. */
    flow_set_cc4_ctrl(current_core, 0);
    flow_set_halt_events(current_core, false);
    FLOW_CTLR_L2FLUSH_CONTROL_0 = 0;
    flow_set_csr(current_core, 2);

    /* Save core context. */
    set_core_entrypoint_and_argument(current_core, entrypoint, argument);
    save_current_core_context();
    set_current_core_inactive();
    call_with_stack_pointer(get_smc_core012_stack_address(), save_se_and_power_down_cpu);

    generic_panic();
}
