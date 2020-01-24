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
#include "emc.h"
#include "pmc.h"
#include "timer.h"

static void emc_trigger_timing_update(void) {
    EMC_TIMING_CONTROL_0 = 1;
    while (EMC_EMC_STATUS_0 & 0x800000) {
        /* Wait until TIMING_UPDATE_STALLED is unset. */
    }
}

/* Puts DRAM into self refresh mode. */
void emc_put_dram_in_self_refresh_mode(void) {
    /* Verify CH1_ENABLE [PMC]. */
    if (!(EMC_FBIO_CFG7_0 & 4)) {
        reboot();
    }

    /* Clear config. */
    EMC_CFG_0 = 0;
    emc_trigger_timing_update();
    timer_wait(5);

    /* Set calibration intervals. */
    EMC_ZCAL_INTERVAL_0 = 0;
    EMC_AUTO_CAL_CONFIG_0 = 0x600; /* AUTO_CAL_MEASURE_STALL | AUTO_CAL_UPDATE_STALL */

    /* If EMC0 mirror is set, clear digital DLL. */
    if (EMC0_CFG_DIG_DLL_0 & 1) {
        EMC_CFG_DIG_DLL_0 &= 0xFFFFFFFE;
        emc_trigger_timing_update();
        while (EMC0_CFG_DIG_DLL_0 & 1) { /* Wait for EMC0 to clear. */ }
        while (EMC1_CFG_DIG_DLL_0 & 1) { /* Wait for EMC1 to clear. */ }
    } else {
        emc_trigger_timing_update();
    }

    /* Stall all transactions to DRAM. */
    EMC_REQ_CTRL_0 = 3; /* STALL_ALL_WRITES | STALL_ALL_READS. */
    while (!(EMC0_EMC_STATUS_0 & 4)) { /* Wait for NO_OUTSTANDING_TRANSACTIONS for EMC0. */ }
    while (!(EMC1_EMC_STATUS_0 & 4)) { /* Wait for NO_OUTSTANDING_TRANSACTIONS for EMC1. */ }

    /* Enable Self-Refresh Mode. */
    EMC_SELF_REF_0 |= 1;


    /* Wait until we see the right devices in self refresh mode. */
    uint32_t num_populated_devices = 1;
    if (EMC_ADR_CFG_0) {
        num_populated_devices = 3;
    }

    while (((EMC0_EMC_STATUS_0 >> 8) & 3) != num_populated_devices) { /* Wait for EMC0 DRAM_IN_SELF_REFRESH to be correct. */ }
    while (((EMC1_EMC_STATUS_0 >> 8) & 3) != num_populated_devices) { /* Wait for EMC1 DRAM_IN_SELF_REFRESH to be correct. */ }
}
