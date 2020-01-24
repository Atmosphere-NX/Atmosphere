/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "sc7.h"
#include "i2c.h"
#include "pmc.h"
#include "emc.h"
#include "timer.h"

#define CACHE_CTRL MAKE_REG32(0x50040000)

#define PRI_ICTLR_COP_IER_CLR_0   MAKE_REG32(0x60004038)
#define SEC_ICTLR_COP_IER_CLR_0   MAKE_REG32(0x60004138)
#define TRI_ICTLR_COP_IER_CLR_0   MAKE_REG32(0x60004238)
#define QUAD_ICTLR_COP_IER_CLR_0  MAKE_REG32(0x60004338)
#define PENTA_ICTLR_COP_IER_CLR_0 MAKE_REG32(0x60004438)
#define HEXA_ICTLR_COP_IER_CLR_0  MAKE_REG32(0x60004538)

void reboot(void) {
    /* Write MAIN_RST */
    APBDEV_PMC_CNTRL_0 = 0x10;
    while (true) {
        /* Wait for reboot. */
    }
}


static void set_pmc_dpd_io_pads(void) {
    /* Read val from EMC_PMC scratch, configure accordingly. */
    uint32_t emc_pmc_val = EMC_PMC_SCRATCH3_0;
    APBDEV_PMC_DDR_CNTRL_0 = emc_pmc_val & 0x7FFFF;
    if (emc_pmc_val & 0x40000000) {
        APBDEV_PMC_WEAK_BIAS_0 = 0x7FFF0000;
    }
    /* Request to put pads in Deep Power Down. */
    APBDEV_PMC_IO_DPD3_REQ_0 = 0x8FFFFFFF;
    while (APBDEV_PMC_IO_DPD3_STATUS_0 != 0xFFFFFFF) { /* Wait a while. */ }
    spinlock_wait(32);
    APBDEV_PMC_IO_DPD4_REQ_0 = 0x8FFFFFFF;
    while (APBDEV_PMC_IO_DPD4_STATUS_0 != 0xFFF1FFF) { /* Wait a while. */ }
    spinlock_wait(32);
}

void sc7_entry_main(void) {
    /* Disable the BPMP Cache. */
    CACHE_CTRL |= 0xC00;

    /* Wait until the CPU Rail is turned off. */
    while (APBDEV_PMC_PWRGATE_STATUS_0 & 1) { /* Wait for TrustZone to finish. */ }

    /* Clamp the CPU Rail. */
    APBDEV_PMC_SET_SW_CLAMP_0 |= 0x1;
    while (!(APBDEV_PMC_CLAMP_STATUS_0 & 1)) { /* Wait for CPU Rail to be clamped. */ }

    /* Waste some time. */
    spinlock_wait(10);

    /* Reset device 27 over I2C, then wait a while. */
    i2c_init();
    i2c_send_reset_cmd();
    timer_wait(700);

    /* Clear Interrupt Enable for BPMP in all ICTLRs. */
    PRI_ICTLR_COP_IER_CLR_0   = 0xFFFFFFFF;
    SEC_ICTLR_COP_IER_CLR_0   = 0xFFFFFFFF;
    TRI_ICTLR_COP_IER_CLR_0   = 0xFFFFFFFF;
    QUAD_ICTLR_COP_IER_CLR_0  = 0xFFFFFFFF;
    PENTA_ICTLR_COP_IER_CLR_0 = 0xFFFFFFFF;
    HEXA_ICTLR_COP_IER_CLR_0  = 0xFFFFFFFF;

    /* Write EMC's DRAM op into PMC scratch. */
    if ((EMC_FBIO_CFG5_0 & 3) != 1) {
        /* If DRAM_TYPE != LPDDR4, something's gone wrong. Reboot. */
        reboot();
    }
    /* Write MRW3_OP into scratch. */
    APBDEV_PMC_SCRATCH18_0 = (APBDEV_PMC_SCRATCH18_0 & 0xFFFFFF3F) | (EMC_MRW3_0 & 0xC0);
    uint32_t mrw3_op = ((EMC_MRW3_0 & 0xC0) << 8) | (EMC_MRW3_0 & 0xC0);
    APBDEV_PMC_SCRATCH12_0 = (APBDEV_PMC_SCRATCH12_0 & 0xFFFF3F3F) | mrw3_op;
    APBDEV_PMC_SCRATCH13_0 = (APBDEV_PMC_SCRATCH13_0 & 0xFFFF3F3F) | mrw3_op;

    /* Ready DRAM for deep sleep. */
    emc_put_dram_in_self_refresh_mode();

    /* Setup LPDDR MRW based on device config. */
    EMC_MRW_0 = 0x88110000;
    if (EMC_ADR_CFG_0 & 1) {
        EMC_MRW_0 = 0x48110000;
    }

    /* Put IO pads in Deep Power Down. */
    set_pmc_dpd_io_pads();

    /* Enable pad sampling during deep sleep. */
    APBDEV_PMC_DPD_SAMPLE_0 |= 1;

    /* Waste some more time. */
    spinlock_wait(0x128);

    /* Enter deep sleep. */
    APBDEV_PMC_DPD_ENABLE_0 |= 1;

    while (true) { /* Wait until we're asleep. */ }
}


