/*
 * Copyright (c) 2018-2019 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
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
#include "sc7.h"
#include "masterkey.h"
#include "pmc.h"
#include "se.h"
#include "smc_api.h"
#include "timers.h"
#include "misc.h"
#include "uart.h"
#include "exocfg.h"

#define u8 uint8_t
#define u32 uint32_t
#include "sc7fw_bin.h"
#undef u8
#undef u32

static void configure_battery_hiz_mode(void) {
    clkrst_reboot(CARDEVICE_I2C1);

    if (configitem_is_hiz_mode_enabled() && !i2c_query_ti_charger_bit_7()) {
        /* Configure HiZ mode. */
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
}

static void enable_lp0_wake_events(void) {
    wait(75);
    APBDEV_PMC_CNTRL2_0 |= 0x200; /* Set WAKE_DET_EN. */
    wait(75);
    APBDEV_PM_0 = 0xFFFFFFFF; /* Set all wake events. */
    APBDEV_PMC_WAKE2_STATUS_0 = 0xFFFFFFFF; /* Set all wake events. */
    wait(75);
}

static void notify_pmic_shutdown(void) {
    clkrst_reboot(CARDEVICE_I2C5);
    if (fuse_get_bootrom_patch_version() >= 0x7F) {
        i2c_send_pmic_cpu_shutdown_cmd();
    }
}

static void mitigate_jamais_vu(void) {
    /* Jamais Vu mitigation #1: Ensure all other cores are off. */
    if (APBDEV_PMC_PWRGATE_STATUS_0 & 0xE00) {
        generic_panic();
    }

    /* For debugging, make this check always pass. */
    if ((exosphere_get_target_firmware() < ATMOSPHERE_TARGET_FIRMWARE_400 || (get_debug_authentication_status() & 3) == 3)) {
        FLOW_CTLR_HALT_COP_EVENTS_0 = 0x50000000;
    } else {
        FLOW_CTLR_HALT_COP_EVENTS_0 = 0x40000000;
    }

    /* Jamais Vu mitigation #2: Ensure the BPMP is halted. */
    if (exosphere_get_target_firmware() < ATMOSPHERE_TARGET_FIRMWARE_400 || (get_debug_authentication_status() & 3) == 3) {
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
    if ((CLK_RST_CONTROLLER_RST_DEVICES_H_0 & 0x4000006) != 0x4000006) {
        generic_panic();
    }
}

static void configure_pmc_for_deep_powerdown(void) {
    APBDEV_PMC_SCRATCH0_0 = 1;
    APBDEV_PMC_DPD_ENABLE_0 |= 2;
}

static void setup_bpmp_sc7_firmware(void) {
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
    for (unsigned int i = 0; i < sc7fw_bin_size; i += 4) {
        write32le(lp0_entry_code, i, read32le(sc7fw_bin, i));
    }
        
    flush_dcache_range(lp0_entry_code, lp0_entry_code + sc7fw_bin_size);

    /* Take the BPMP out of reset. */
    MAKE_CAR_REG(0x304) = 2;

    /* Start executing BPMP firmware. */
    FLOW_CTLR_HALT_COP_EVENTS_0 = 0;
}

static void configure_flow_regs_for_sleep(void) {
    unsigned int current_core = get_core_id();
    flow_set_cc4_ctrl(current_core, 0);
    flow_set_halt_events(current_core, false);
    FLOW_CTLR_L2FLUSH_CONTROL_0 = 0;
    flow_set_csr(current_core, 2);
}

static void save_tzram_state(void) {
    /* TODO: Remove set suspend call once exo warmboots fully */
    set_suspend_for_debug();
    uint32_t tzram_cmac[0x4] = {0};

    uint8_t *tzram_encryption_dst = (uint8_t *)(LP0_ENTRY_GET_RAM_SEGMENT_ADDRESS(LP0_ENTRY_RAM_SEGMENT_ID_ENCRYPTED_TZRAM));
    uint8_t *tzram_encryption_src = (uint8_t *)(LP0_ENTRY_GET_RAM_SEGMENT_ADDRESS(LP0_ENTRY_RAM_SEGMENT_ID_CURRENT_TZRAM));
    if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_500) {
        tzram_encryption_src += 0x2000ull;
    }
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
    for (unsigned int i = 0; i < LP0_TZRAM_SAVE_SIZE; i += 4) {
        write32le(tzram_store_address, i, read32le(tzram_encryption_dst, i));
    }
    
    flush_dcache_range(tzram_store_address, tzram_store_address + LP0_TZRAM_SAVE_SIZE);

    /* Compute CMAC. */
    se_compute_aes_256_cmac(KEYSLOT_SWITCH_LP0TZRAMKEY, tzram_cmac, sizeof(tzram_cmac), tzram_encryption_src, LP0_TZRAM_SAVE_SIZE);
    
    /* Write CMAC, lock registers. */
    APBDEV_PMC_SECURE_SCRATCH112_0 = tzram_cmac[0];
    APBDEV_PMC_SECURE_SCRATCH113_0 = tzram_cmac[1];
    APBDEV_PMC_SECURE_SCRATCH114_0 = tzram_cmac[2];
    APBDEV_PMC_SECURE_SCRATCH115_0 = tzram_cmac[3];
    APBDEV_PMC_SEC_DISABLE8_0 = 0x550000;

    /* Perform pre-2.0.0 PMC writes. */
    if (exosphere_get_target_firmware() < ATMOSPHERE_TARGET_FIRMWARE_200) {
        /* TODO: Give these writes appropriate defines in pmc.h */

        /* Save Encrypted context location + lock scratch register. */
        MAKE_REG32(PMC_BASE + 0x360) = WARMBOOT_GET_RAM_SEGMENT_PA(WARMBOOT_RAM_SEGMENT_ID_TZRAM);
        MAKE_REG32(PMC_BASE + 0x2D8) = 0x10000;

        /* Save Encryption parameters (where to copy TZRAM to, source, destination, size) */
        MAKE_REG32(PMC_BASE + 0x340) = LP0_ENTRY_GET_RAM_SEGMENT_PA(LP0_ENTRY_RAM_SEGMENT_ID_CURRENT_TZRAM);
        MAKE_REG32(PMC_BASE + 0x344) = 0;
        MAKE_REG32(PMC_BASE + 0x348) = LP0_ENTRY_GET_RAM_SEGMENT_PA(LP0_ENTRY_RAM_SEGMENT_ID_CURRENT_TZRAM);
        MAKE_REG32(PMC_BASE + 0x34C) = 0;
        MAKE_REG32(PMC_BASE + 0x350) = LP0_ENTRY_GET_RAM_SEGMENT_PA(LP0_ENTRY_RAM_SEGMENT_ID_CURRENT_TZRAM);
        MAKE_REG32(PMC_BASE + 0x354) = LP0_TZRAM_SAVE_SIZE;

        /* Lock scratch registers. */
        MAKE_REG32(PMC_BASE + 0x2D8) = 0x555;
    }
}

static void save_se_state(void) {
    /* Save security engine state. */
    uint8_t *se_state_dst = (uint8_t *)(WARMBOOT_GET_RAM_SEGMENT_ADDRESS(WARMBOOT_RAM_SEGMENT_ID_SE_STATE));
    se_check_error_status_reg();
    se_set_in_context_save_mode(true);
    se_save_context(KEYSLOT_SWITCH_SRKGENKEY, KEYSLOT_SWITCH_RNGKEY, se_state_dst);
    flush_dcache_range(se_state_dst, se_state_dst + 0x840);
    APBDEV_PMC_SCRATCH43_0 = (uint32_t)(WARMBOOT_GET_RAM_SEGMENT_PA(WARMBOOT_RAM_SEGMENT_ID_SE_STATE));
    se_set_in_context_save_mode(false);
    se_check_error_status_reg();
}

/* Save security engine, and go to sleep. */
void save_se_and_power_down_cpu(void) {
    /* Save context for warmboot to restore. */
    save_tzram_state();
    save_se_state();
    
    /* Patch the bootrom to disable warmboot signature checks. */
    MAKE_REG32(PMC_BASE + 0x118) = 0x2202E012;
    MAKE_REG32(PMC_BASE + 0x11C) = 0x6001DC28;

    if (!configitem_is_retail()) {
        uart_send(UART_A, "OYASUMI", 8);
    }
    
    finalize_powerdown();
}

uint32_t cpu_suspend(uint64_t power_state, uint64_t entrypoint, uint64_t argument) {
    /* TODO: 6.0.0 introduces heavy deja vu mitigations. */
    /* Exosphere may want to implement these. */
    
    /* Ensure SMC call is to enter deep sleep. */
    if ((power_state & 0x17FFF) != 0x1001B) {
        return 0xFFFFFFFD;
    }

    /* Perform I2C comms with TI charger if required. */
    configure_battery_hiz_mode();

    /* Enable LP0 Wake Event Detection. */
    enable_lp0_wake_events();

    /* Alert the PMC of an iminent shutdown. */
    notify_pmic_shutdown();

    /* Validate that the shutdown has correct context. */
    if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_200) {
        mitigate_jamais_vu();
    }

    /* Signal to bootrom the next reset should be a warmboot. */
    configure_pmc_for_deep_powerdown();

    /* Ensure that BPMP SC7 firmware is active. */
    if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_200) {
        setup_bpmp_sc7_firmware();
    }

    /* Prepare the current core for sleep. */
    configure_flow_regs_for_sleep();
    
    /* Save core context. */
    set_core_entrypoint_and_argument(get_core_id(), entrypoint, argument);
    save_current_core_context();
    set_current_core_inactive();

    /* Ensure that other cores are already asleep. */
    if (!(APBDEV_PMC_PWRGATE_STATUS_0 & 0xE00)) {
        if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_200) {
            call_with_stack_pointer(get_smc_core012_stack_address(), save_se_and_power_down_cpu);
        } else {
            save_se_and_power_down_cpu();
        }
    }

    generic_panic();
}
