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
 
#include "utils.h"
#include "mmu.h"
#include "memory_map.h"
#include "cpu_context.h"
#include "bootconfig.h"
#include "configitem.h"
#include "masterkey.h"
#include "bootup.h"
#include "smc_api.h"
#include "exocfg.h"

#include "se.h"
#include "mc.h"
#include "car.h"
#include "i2c.h"
#include "misc.h"
#include "uart.h"
#include "interrupt.h"

#include "pmc.h"

uintptr_t get_warmboot_main_stack_address(void) {
    return TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGEMENT_ID_SECMON_EVT) + 0x780;
}

static void warmboot_configure_hiz_mode(void) {
    /* Enable input to I2C1 */
    PINMUX_AUX_GEN1_I2C_SCL_0 = 0x40;
    PINMUX_AUX_GEN1_I2C_SDA_0 = 0x40;

    clkrst_reboot(CARDEVICE_I2C1);
    i2c_init(0);
    i2c_clear_ti_charger_bit_7();
    clkrst_disable(CARDEVICE_I2C1);
}

void __attribute__((noreturn)) warmboot_main(void) {
    /*
        This function and its callers are reached in either of the following events, under normal conditions:
            - warmboot (core 3)
            - cpu_on
    */
    
    if (is_core_active(get_core_id())) {
        panic(0xF7F00009); /* invalid CPU context */
    }

    /* IRAM C+D identity mapping has actually been removed on coldboot but we don't really care */
    /* For our crt0 to work, this doesn't actually unmap TZRAM */
    identity_unmap_iram_cd_tzram();

    /* On warmboot (not cpu_on) only */
    if (VIRT_MC_SECURITY_CFG3 == 0) {
        /* N only does this on dev units, but we will do it unconditionally. */
        {
            uart_select(UART_A);
            clkrst_reboot(CARDEVICE_UARTA);
            uart_init(UART_A, 115200);
        }
        
        if (!configitem_is_retail()) {
            uart_send(UART_A, "OHAYO", 6);
            uart_wait_idle(UART_A, UART_VENDOR_STATE_TX_IDLE);
        }

        /* Sanity check the Security Engine. */
        se_verify_flags_cleared();

        /* Initialize interrupts. */
        intr_initialize_gic_nonsecure();

        bootup_misc_mmio();

        /* Make PMC (2.x+), MC (4.x+) registers secure-only */
        secure_additional_devices();

        if ((exosphere_get_target_firmware() < ATMOSPHERE_TARGET_FIRMWARE_400) || 
           ((exosphere_get_target_firmware() < ATMOSPHERE_TARGET_FIRMWARE_800) && configitem_get_hardware_type() == 0) ||
           (configitem_is_hiz_mode_enabled())) {
            warmboot_configure_hiz_mode();
        }

        clear_user_smc_in_progress();

        if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_400) {
            setup_4x_mmio();
        }
    }

    setup_current_core_state();

    /* Update SCR_EL3 depending on value in Bootconfig. */
    set_extabt_serror_taken_to_el3(bootconfig_take_extabt_serror_to_el3());
    core_jump_to_lower_el();
}
