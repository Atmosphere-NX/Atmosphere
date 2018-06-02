#include <stdint.h>
#include <stdbool.h>

#include "utils.h"
#include "bootup.h"

#include "fuse.h"
#include "bpmp.h"
#include "flow.h"
#include "pmc.h"
#include "mc.h"
#include "car.h"
#include "se.h"
#include "masterkey.h"
#include "configitem.h"
#include "timers.h"
#include "misc.h"
#include "bpmp.h"
#include "sysreg.h"
#include "interrupt.h"
#include "cpu_context.h"
#include "actmon.h"
#include "sysctr0.h"
#include "exocfg.h"

#include "mmu.h"
#include "arm.h"
#include "memory_map.h"
#include "synchronization.h"

static bool g_has_booted_up = false;

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

    /* TODO: Why does this DRAM write occur? */
    if (!g_has_booted_up && exosphere_get_target_firmware() >= EXOSPHERE_TARGET_FIRMWARE_400) {
        /* 4.x writes this magic number into DRAM. Why? */
        (*(volatile uint32_t *)(0x8005FFFC)) = 0xC0EDBBCC;
    }

    /* Todo: What? */
    MAKE_TIMERS_REG(0x1A4) = 0xF1E0;

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
    if (exosphere_get_target_firmware() == EXOSPHERE_TARGET_FIRMWARE_100) {
        /* TODO: Switch these to use the enum. */
        APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG0_0 = 0x500244;
        APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG1_0 = 0xA3700000;
        APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG2_0 = 0x304;
    } else {
        /* Mark SATA_AUX, DTV, QSPI, SE, SATA, LA secure only. */
        APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG0_0 = APB_SSER0_SATA_AUX | APB_SSER0_DTV | APB_SSER0_QSPI | APB_SSER0_SE | APB_SSER0_SATA | APB_SSER0_LA;

        /* By default, mark SPI1, SPI2, SPI3, SPI5, SPI6, I2C6 secure only. */
        uint32_t sec_disable_1 = APB_SSER1_SPI1 | APB_SSER1_SPI2 | APB_SSER1_SPI3 | APB_SSER1_SPI5 | APB_SSER1_SPI6 | APB_SSER1_I2C6;
        /* By default, mark SDMMC3, DDS, DP2 secure only. */
        uint32_t sec_disable_2 = APB_SSER2_SDMMC3 | APB_SSER2_DDS | APB_SSER2_DP2;
        uint64_t hardware_type = configitem_get_hardware_type();
        if (hardware_type != 1) {
            /* Also mark I2C4 secure only, */
            sec_disable_1 |= APB_SSER1_I2C4;
        }
        if (hardware_type != 0 && exosphere_get_target_firmware() >= EXOSPHERE_TARGET_FIRMWARE_400) {
            /* Starting on 4.x on non-dev units, mark UARTB, UARTC, SPI4, I2C3 secure only. */
            sec_disable_1 |= APB_SSER1_UART_B | APB_SSER1_UART_C | APB_SSER1_SPI4 | APB_SSER1_I2C3;
            /* Starting on 4.x on non-dev units, mark SDMMC1 secure only. */
            sec_disable_2 |= APB_SSER2_SDMMC1;
        }
        APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG1_0 = sec_disable_1;
        APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG2_0 = sec_disable_2;
    }

    /* TODO: What are these MC reg writes? */
    MAKE_MC_REG(0x228) = 0xFFFFFFFF;
    MAKE_MC_REG(0x22C) = 0xFFFFFFFF;
    MAKE_MC_REG(0x230) = 0xFFFFFFFF;
    MAKE_MC_REG(0x234) = 0xFFFFFFFF;
    MAKE_MC_REG(0xB98) = 0xFFFFFFFF;
    if (exosphere_get_target_firmware() >= EXOSPHERE_TARGET_FIRMWARE_400) {
        MAKE_MC_REG(0x038) = 0xE;
    } else {
        MAKE_MC_REG(0x038) = 0x0;
    }
    MAKE_MC_REG(0x03C) = 0;
    MAKE_MC_REG(0x9E0) = 0;
    MAKE_MC_REG(0x9E4) = 0;
    MAKE_MC_REG(0x9E8) = 0;
    MAKE_MC_REG(0x9EC) = 0;
    MAKE_MC_REG(0x9F0) = 0;
    MAKE_MC_REG(0x9F4) = 0;
    if (exosphere_get_target_firmware() >= EXOSPHERE_TARGET_FIRMWARE_400) {
        MAKE_MC_REG(0x01C) = 0;
    }
    MAKE_MC_REG(0x020) = 0;
    MAKE_MC_REG(0x014) = 0x30000030;
    MAKE_MC_REG(0x018) = 0x2800003F;
    (void)(MAKE_MC_REG(0x014));
    MAKE_MC_REG(0x034) = 0;
    (void)(MAKE_MC_REG(0x014));
    MAKE_MC_REG(0x030) = 0;
    (void)(MAKE_MC_REG(0x014));
    MAKE_MC_REG(0x010) = 1;
    (void)(MAKE_MC_REG(0x014));
    
    /* Clear RESET Vector, setup CPU Secure Boot RESET Vectors. */
    uint32_t reset_vec = TZRAM_GET_SEGMENT_PA(TZRAM_SEGMENT_ID_WARMBOOT_CRT0_AND_MAIN);
    EVP_CPU_RESET_VECTOR_0 = 0;
    SB_AA64_RESET_LOW_0 = reset_vec | 1;
    SB_AA64_RESET_HIGH_0 = 0;

    /* Lock Non-Secure writes to Secure Boot RESET Vector. */
    SB_CSR_0 = 2;

    /* Setup PMC Secure Scratch RESET Vector for warmboot. */
    APBDEV_PMC_SECURE_SCRATCH34_0 = reset_vec;
    APBDEV_PMC_SECURE_SCRATCH35_0 = 0;
    APBDEV_PMC_SEC_DISABLE3_0 = 0x500000;

    /* Setup FIQs. */


    /* And assign "se_operation_completed" to Interrupt 0x5A. */
    intr_set_priority(INTERRUPT_ID_SECURITY_ENGINE, 0);
    intr_set_group(INTERRUPT_ID_SECURITY_ENGINE, 0);
    intr_set_enabled(INTERRUPT_ID_SECURITY_ENGINE, 1);
    intr_set_cpu_mask(INTERRUPT_ID_SECURITY_ENGINE, 8);
    intr_set_edge_level(INTERRUPT_ID_SECURITY_ENGINE, 0);
    if (exosphere_get_target_firmware() >= EXOSPHERE_TARGET_FIRMWARE_400) {
        intr_set_priority(INTERRUPT_ID_ACTIVITY_MONITOR_4X, 0);
        intr_set_group(INTERRUPT_ID_ACTIVITY_MONITOR_4X, 0);
        intr_set_enabled(INTERRUPT_ID_ACTIVITY_MONITOR_4X, 1);
        intr_set_cpu_mask(INTERRUPT_ID_ACTIVITY_MONITOR_4X, 8);
        intr_set_edge_level(INTERRUPT_ID_ACTIVITY_MONITOR_4X, 0);
    }

    if (!g_has_booted_up) {
        intr_register_handler(INTERRUPT_ID_SECURITY_ENGINE, se_operation_completed);
        if (exosphere_get_target_firmware() >= EXOSPHERE_TARGET_FIRMWARE_400) {
            intr_register_handler(INTERRUPT_ID_ACTIVITY_MONITOR_4X, actmon_interrupt_handler);
        }
        for (unsigned int core = 1; core < NUM_CPU_CORES; core++) {
            set_core_is_active(core, false);
        }
        g_has_booted_up = true;
    } else if (exosphere_get_target_firmware() < EXOSPHERE_TARGET_FIRMWARE_400) {
        /* TODO: What are these MC reg writes? */
        MAKE_MC_REG(0x65C) = 0xFFFFF000;
        MAKE_MC_REG(0x660) = 0;
        MAKE_MC_REG(0x964) |= 1;
        CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD_0 &= 0xFFF7FFFF;
    }
}

void setup_4x_mmio(void) {
    /* TODO: What are these MC reg writes? */
    MAKE_MC_REG(0x65C) = 0xFFFFF000;
    MAKE_MC_REG(0x660) = 0;
    MAKE_MC_REG(0x964) |= 1;
    CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD_0 &= 0xFFF7FFFF;
    /* TODO: What are these PMC scratch writes? */
    APBDEV_PMC_SECURE_SCRATCH51_0 = (APBDEV_PMC_SECURE_SCRATCH51_0 & 0xFFFF8000) | 0x4000;
    APBDEV_PMC_SECURE_SCRATCH16_0 &= 0x3FFFFFFF;
    APBDEV_PMC_SECURE_SCRATCH55_0 = (APBDEV_PMC_SECURE_SCRATCH55_0 & 0xFF000FFF) | 0x1000;
    APBDEV_PMC_SECURE_SCRATCH74_0 = 0x40008000;
    APBDEV_PMC_SECURE_SCRATCH75_0 = 0x40000;
    APBDEV_PMC_SECURE_SCRATCH76_0 = 0x0;
    APBDEV_PMC_SECURE_SCRATCH77_0 = 0x0;
    APBDEV_PMC_SECURE_SCRATCH78_0 = 0x0;
    APBDEV_PMC_SECURE_SCRATCH99_0 = 0x40008000;
    APBDEV_PMC_SECURE_SCRATCH100_0 = 0x40000;
    APBDEV_PMC_SECURE_SCRATCH101_0 = 0x0;
    APBDEV_PMC_SECURE_SCRATCH102_0 = 0x0;
    APBDEV_PMC_SECURE_SCRATCH103_0 = 0x0;
    APBDEV_PMC_SECURE_SCRATCH39_0 = (APBDEV_PMC_SECURE_SCRATCH39_0 & 0xF8000000) | 0x88;
    /* TODO: Do we want to bother locking the secure scratch registers? */
    /* 4.x Jamais Vu mitigations. */
    /* Overwrite exception vectors. */
    BPMP_VECTOR_RESET = BPMP_MITIGATION_RESET_VAL;
    BPMP_VECTOR_UNDEF = BPMP_MITIGATION_RESET_VAL;
    BPMP_VECTOR_SWI = BPMP_MITIGATION_RESET_VAL;
    BPMP_VECTOR_PREFETCH_ABORT = BPMP_MITIGATION_RESET_VAL;
    BPMP_VECTOR_DATA_ABORT = BPMP_MITIGATION_RESET_VAL;
    BPMP_VECTOR_UNK = BPMP_MITIGATION_RESET_VAL;
    BPMP_VECTOR_IRQ = BPMP_MITIGATION_RESET_VAL;
    BPMP_VECTOR_FIQ = BPMP_MITIGATION_RESET_VAL;
    /* Disable AHB arbitration for the BPMP. */
    AHB_ARBITRATION_DISABLE_0 |= 2;
    /* Set SMMU for BPMP/APB-DMA to point to TZRAM. */
    MC_SMMU_PTB_ASID_0 = 1;
    (void)(MAKE_MC_REG(0x014));
    MC_SMMU_PTB_DATA_0 = 0x70012;
    MC_SMMU_AVPC_ASID_0 = 0x80000001;
    MC_SMMU_PPCS1_ASID_0 = 0x80000001;
    (void)(MAKE_MC_REG(0x014));
    MAKE_MC_REG(0x34) = 0;
    (void)(MAKE_MC_REG(0x014));
    MAKE_MC_REG(0x30) = 0;
    (void)(MAKE_MC_REG(0x014));
    /* Wait for the BPMP to halt. */
    while ((FLOW_CTLR_HALT_COP_EVENTS_0 >> 29) != 2) {
        wait(1);
    }
    /* If not in a debugging context, setup the activity monitor. */
    if ((get_debug_authentication_status() & 3) != 3) {
        FLOW_CTLR_HALT_COP_EVENTS_0 = 0x40000000;
        clkrst_reboot(CARDEVICE_ACTMON);
        /* Sample every microsecond. */
        ACTMON_GLB_PERIOD_CTRL_0 = 0x100;
        /* Fire interrupt every wakeup. */
        ACTMON_COP_UPPER_WMARK_0 = 0;
        /* Cause a panic() on BPMP wakeup. */
        actmon_set_callback(actmon_on_bpmp_wakeup);
        /* Enable interrupt when above watermark, periodic sampling. */
        ACTMON_COP_CTRL_0 = 0xC0040000;
    }
}

void setup_current_core_state(void) {
    uint64_t temp_reg;

    /* Setup system registers. */
    SET_SYSREG(spsr_el3, 0b1111 << 6 | 0b1001); /* use EL2h+DAIF set initially, may be overwritten later. Not in official code */

    SET_SYSREG(actlr_el3, 0x73ull);
    SET_SYSREG(actlr_el2, 0x73ull);
    SET_SYSREG(hcr_el2, 0x80000000ull);
    SET_SYSREG(dacr32_el2, 0xFFFFFFFFull);
    SET_SYSREG(sctlr_el1, 0xC50838ull);
    SET_SYSREG(sctlr_el2, 0x30C50838ull);

    __isb();

    SET_SYSREG(cntfrq_el0, MAKE_SYSCTR0_REG(0x20)); /* TODO: Reg name. */
    SET_SYSREG(cnthctl_el2, 3ull);

    __isb();

    /* Setup Interrupts, flow. */
    flow_clear_csr0_and_events(get_core_id());
    intr_initialize_gic();
    intr_set_priority(INTERRUPT_ID_1C, 0);
    intr_set_group(INTERRUPT_ID_1C, 0);
    intr_set_enabled(INTERRUPT_ID_1C, 1);

    /* Restore current core context. */
    restore_current_core_context();
}

void identity_unmap_iram_cd_tzram(void) {
    /* See also: configure_ttbls (in coldboot_init.c). */
    /*uintptr_t *mmu_l1_tbl = (uintptr_t *)(TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGEMENT_ID_SECMON_EVT) + 0x800 - 64);
    uintptr_t *mmu_l2_tbl = (uintptr_t *)TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGMENT_ID_L2_TRANSLATION_TABLE);*/
    uintptr_t *mmu_l3_tbl = (uintptr_t *)TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGMENT_ID_L3_TRANSLATION_TABLE);

    mmu_unmap_range(3, mmu_l3_tbl, IDENTITY_GET_MAPPING_ADDRESS(IDENTITY_MAPPING_IRAM_A_CCRT0), IDENTITY_GET_MAPPING_SIZE(IDENTITY_MAPPING_IRAM_A_CCRT0));
    mmu_unmap_range(3, mmu_l3_tbl, IDENTITY_GET_MAPPING_ADDRESS(IDENTITY_MAPPING_IRAM_CD), IDENTITY_GET_MAPPING_SIZE(IDENTITY_MAPPING_IRAM_CD));
    /*mmu_unmap_range(3, mmu_l3_tbl, IDENTITY_GET_MAPPING_ADDRESS(IDENTITY_MAPPING_TZRAM), IDENTITY_GET_MAPPING_SIZE(IDENTITY_MAPPING_TZRAM));

    mmu_unmap(2, mmu_l2_tbl, 0x40000000);
    mmu_unmap(2, mmu_l2_tbl, 0x7C000000);

    mmu_unmap(1, mmu_l1_tbl, 0x40000000);*/

    tlb_invalidate_all_inner_shareable();
}

void secure_additional_devices(void) {
    if (exosphere_get_target_firmware() >= EXOSPHERE_TARGET_FIRMWARE_200) {
        APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG0_0 |= APB_SSER0_PMC; /* make PMC secure-only (2.x+) */
        if (exosphere_get_target_firmware() >= EXOSPHERE_TARGET_FIRMWARE_400) {
            APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG1_0 |= APB_SSER1_MC0 | APB_SSER1_MC1 | APB_SSER1_MCB;  /* make MC0, MC1, MCB secure-only (4.x+) */
        }
    }
}

void set_extabt_serror_taken_to_el3(bool taken_to_el3) {
    uint64_t temp_scr_el3;
    __asm__ __volatile__ ("mrs %0, scr_el3" : "=r"(temp_scr_el3) :: "memory");

    temp_scr_el3 &= 0xFFFFFFF7;
    temp_scr_el3 |= taken_to_el3 ? 8 : 0;

    __asm__ __volatile__ ("msr scr_el3, %0" :: "r"(temp_scr_el3) : "memory");
    __isb();
}
