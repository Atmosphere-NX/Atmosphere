#include <stdint.h>

#include "utils.h"
#include "bootup.h"

#include "fuse.h"
#include "flow.h"
#include "pmc.h"
#include "mc.h"
#include "se.h"
#include "masterkey.h"
#include "configitem.h"
#include "misc.h"

void bootup_misc_mmio(void) {
    /* Initialize Fuse registers. */
    fuse_init();
    
    /* Verify Security Engine sanity. */
    se_set_in_context_save_mode(false);
    /* TODO: se_verify_keys_unreadable(); */
    se_validate_stored_vector();

    for (unsigned int i = 0; i < KEYSLOT_SWITCH_SESSIONKEY; i++) {
        clear_aes_keyslot(i);
    }

    for (unsigned int i = 0; i < KEYSLOT_RSA_MAX; i++) {
        clear_rsa_keyslot(i);
    }
    se_initialize_rng(KEYSLOT_SWITCH_RNGKEY);
    se_generate_random_key(KEYSLOT_SWITCH_SRKGENKEY, KEYSLOT_SWITCH_RNGKEY);
    se_generate_srk(KEYSLOT_SWITCH_SRKGENKEY);

    FLOW_CTLR_BPMP_CLUSTER_CONTROL_0 = 4; /* ACTIVE_CLUSTER_LOCK. */
    FLOW_CTLR_FLOW_DBG_QUAL_0 = 0x10000000; /* Enable FIQ2CCPLEX */
    
    /* Disable Deep Power Down. */
    APBDEV_PMC_DPD_ENABLE_0 = 0;

    /* Setup MC. */
    /* TODO: What are these MC reg writes? */
    MAKE_MC_REG(0x984) = 1;
    MAKE_MC_REG(0x648) = 0;
    MAKE_MC_REG(0x64C) = 0;
    MAKE_MC_REG(0x650) = 1;
    MAKE_MC_REG(0x670) = 0;
    MAKE_MC_REG(0x674) = 0;
    MAKE_MC_REG(0x678) = 1;
    MAKE_MC_REG(0x9A0) = 0;
    MAKE_MC_REG(0x9A4) = 0;
    MAKE_MC_REG(0x9A8) = 0;
    MAKE_MC_REG(0x9AC) = 1;
    MC_SECURITY_CFG0_0 = 0;
    MC_SECURITY_CFG1_0 = 0;
    MC_SECURITY_CFG3_0 = 3;
    configure_default_carveouts();

    /* Mark registers secure world only. */
    /* Mark SATA_AUX, DTV, QSPI, SE, SATA, LA secure only. */
    APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG0_0 = 0x504244;

    /* By default, mark SPI1, SPI2, SPI3, SPI5, SPI6, I2C6 secure only. */
    uint32_t sec_disable_1 = 0x83700000;
    /* By default, mark SDMMC3, DDS, DP2 secure only. */
    uint32_t sec_disable_2 = 0x304;
    uint64_t hardware_type = configitem_get_hardware_type();
    if (hardware_type != 1) {
        /* Also mark I2C5 secure only, */
        sec_disable_1 |= 0x20000000;
    }
    if (hardware_type != 0 && mkey_get_revision() >= MASTERKEY_REVISION_400_CURRENT) {
        /* Starting on 4.x on non-dev units, mark UARTB, UARTC, SPI4, I2C3 secure only. */
        sec_disable_1 |= 0x10806000;
        /* Starting on 4.x on non-dev units, mark SDMMC1 secure only. */
        sec_disable_2 |= 1;
    }
    APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG1_0 = sec_disable_1;
    APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG2_0 = sec_disable_2;


    /* TODO */
    /* Initialize the PMC secure scratch registers, initialize MISC registers, */
    /* And assign "se_operation_completed" to Interrupt 0x5A. */
}

void setup_4x_mmio(void) {
    /* TODO */
}