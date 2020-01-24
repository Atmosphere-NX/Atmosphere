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
#include "lp0.h"
#include "mc.h"
#include "pmc.h"
#include "misc.h"
#include "fuse.h"
#include "car.h"
#include "emc.h"
#include "cluster.h"
#include "flow.h"
#include "timer.h"
#include "secmon.h"

void reboot(void) {
    /* Write MAIN_RST */
    APBDEV_PMC_CNTRL_0 = 0x10;
    while (true) {
        /* Wait for reboot. */
    }
}

void lp0_entry_main(warmboot_metadata_t *meta) {
    const uint32_t target_firmware = meta->target_firmware;
    /* Before doing anything else, ensure some sanity. */
    if (meta->magic != WARMBOOT_MAGIC || target_firmware > ATMOSPHERE_TARGET_FIRMWARE_MAX) {
        reboot();
    }
    
    /* [4.0.0+] First thing warmboot does is disable BPMP access to memory. */
    if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_400)  {
        disable_bpmp_access_to_dram();
    }
    
    /* Configure debugging depending on FUSE_PRODUCTION_MODE */
    misc_configure_device_dbg_settings();
    
    /* Check for downgrade. */
    /* NOTE: We implemented this as "return false" */
    if (fuse_check_downgrade_status()) {
        reboot();
    }
    
    if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_300) {
        /* Nintendo's firmware checks APBDEV_PMC_SECURE_SCRATCH32_0 against a per-warmboot binary value here. */
        /* We won't bother with that. */
        if (false /* APBDEV_PMC_SECURE_SCRATCH32_0 == WARMBOOT_MAGIC_NUMBER */) {
            reboot();
        }
    }
    
    /* TODO: Check that we're running at the correct physical address. */
    
    /* Setup fuses, disable bypass. */
    fuse_configure_fuse_bypass();
    
    /* Configure oscillators/timing in CAR. */
    car_configure_oscillators();
    
    /* Restore RAM configuration. */
    misc_restore_ram_svop();
    emc_configure_pmacro_training();
    
    /* Setup clock output for all devices, working around mbist bug. */
    car_mbist_workaround();
    
    /* Initialize the CPU cluster. */
    cluster_initialize_cpu();
    
    secmon_restore_to_tzram(target_firmware);
    
    /* Power on the CPU cluster. */
    cluster_power_on_cpu();
    
    /* Nintendo clears most of warmboot.bin out of IRAM here. We're not gonna bother. */
    /* memset( ... ); */
    
    const uint32_t halt_val = (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_400) ? 0x40000000 : 0x50000000;
    while (true) {
        /* Halt the BPMP. */
        FLOW_CTLR_HALT_COP_EVENTS_0 = halt_val;
    }
}
