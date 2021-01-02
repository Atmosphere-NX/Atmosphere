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

#include <string.h>

#include "cluster.h"
#include "di.h"
#include "exocfg.h"
#include "flow.h"
#include "fuse.h"
#include "mc.h"
#include "nxboot.h"
#include "se.h"
#include "smmu.h"
#include "timers.h"
#include "sysreg.h"

/* Determine the current SoC for Mariko specific code. */
static bool is_soc_mariko() {
    return (fuse_get_soc_type() == 1);
}

void nxboot_finish(uint32_t boot_memaddr) {
    bool is_mariko = is_soc_mariko();

    /* Boot up Exosphère. */
    MAILBOX_NX_BOOTLOADER_IS_SECMON_AWAKE = 0;
    MAILBOX_NX_BOOTLOADER_SETUP_STATE = NX_BOOTLOADER_STATE_DRAM_INITIALIZED_4X;

    /* Terminate the display. */
    display_end();

    if (is_mariko) {
        /* Boot CPU0. */
        cluster_boot_cpu0(boot_memaddr);
    } else {
        /* Check if SMMU emulation has been used. */
        uint32_t smmu_magic = *(uint32_t *)(SMMU_AARCH64_PAYLOAD_ADDR + 0xFC);
        if (smmu_magic == 0xDEADC0DE) {
            /* Clear the magic. */
            *(uint32_t *)(SMMU_AARCH64_PAYLOAD_ADDR + 0xFC) = 0;

            /* Pass the boot address to the already running payload. */
            *(uint32_t *)(SMMU_AARCH64_PAYLOAD_ADDR + 0xF0) = boot_memaddr;

            /* Wait a while. */
            mdelay(500);
        } else {
            /* Boot CPU0. */
            cluster_boot_cpu0(boot_memaddr);
        }
    }

    /* Wait for Exosphère to wake up. */
    while (MAILBOX_NX_BOOTLOADER_IS_SECMON_AWAKE == 0) {
        udelay(1);
    }

    /* Signal Exosphère. */
    MAILBOX_NX_BOOTLOADER_SETUP_STATE = NX_BOOTLOADER_STATE_FINISHED_4X;

    /* Halt ourselves in waitevent state. */
    while (1) {
        FLOW_CTLR_HALT_COP_EVENTS_0 = 0x50000000;
    }
}