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
#include "interrupt.h"

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
    if (MC_SECURITY_CFG3_0 == 0) {
        if (!configitem_is_retail()) {
            /* TODO: uart_log("OHAYO"); */
        }

        /* Sanity check the Security Engine. */
        se_verify_flags_cleared();

        /* Initialize interrupts. */
        intr_initialize_gic_nonsecure();

        bootup_misc_mmio();

        /* Make PMC (2.x+), MC (4.x+) registers secure-only */
        secure_additional_devices();

        if (exosphere_get_target_firmware() < EXOSPHERE_TARGET_FIRMWARE_400 || configitem_get_hardware_type() == 0) {
            /* Enable input to I2C1 */
            PINMUX_AUX_GEN1_I2C_SCL_0 = 0x40;
            PINMUX_AUX_GEN1_I2C_SDA_0 = 0x40;

            clkrst_enable(CARDEVICE_I2C1);
            i2c_init(0);
            i2c_clear_ti_charger_bit_7();
            clkrst_disable(CARDEVICE_I2C1);
        }

        clear_user_smc_in_progress();

        if (exosphere_get_target_firmware() >= EXOSPHERE_TARGET_FIRMWARE_400) {
            setup_4x_mmio();
        }
    }

    setup_current_core_state();

    /* Update SCR_EL3 depending on value in Bootconfig. */
    set_extabt_serror_taken_to_el3(bootconfig_take_extabt_serror_to_el3());
    core_jump_to_lower_el();
}
