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
 
#include <stdint.h>

#include "utils.h"
#include "misc.h"
#include "fuse.h"
#include "sysreg.h"
#include "pmc.h"

void misc_configure_device_dbg_settings(void) {
    /* Set APB_MISC_PP_CONFIG_CTL_TBE (enables RTCK daisy-chaining). */
    APB_MISC_PP_CONFIG_CTL_0 = 0x80;
    
    /* Configure JTAG and debug bits. */
    if (FUSE_CHIP_REGS->FUSE_SECURITY_MODE == 1) {
        uint32_t secure_boot_val = 0b0100;      /* Set NIDEN for aarch64. */
        uint32_t pp_config_ctl_val = 0x40;      /* Set APB_MISC_PP_CONFIG_CTL_JTAG. */
        if (APBDEV_PMC_STICKY_BITS_0 & 0x40) {
            pp_config_ctl_val = 0x0;
        } else {
            secure_boot_val = 0b1101;           /* Set SPNIDEN, NIDEN, DBGEN for aarch64. */
        }
        SB_PFCFG_0 = (SB_PFCFG_0 & ~0b1111) | secure_boot_val;      /* Configure debug bits. */
        APB_MISC_PP_CONFIG_CTL_0 |= pp_config_ctl_val;              /* Configure JTAG. */
    }
    
    /* Set HDA_LPBK_DIS if FUSE_SECURITY_MODE is set (disables HDA codec loopback). */
    APBDEV_PMC_STICKY_BITS_0 |= FUSE_CHIP_REGS->FUSE_SECURITY_MODE;
    
    /* Set E_INPUT in PINMUX_AUX_GPIO_PA6_0 (needed by the XUSB and SATA controllers). */
    PINMUX_AUX_GPIO_PA6_0 |= 0x40;
}

void misc_restore_ram_svop(void) {
    /* This sets CFG2TMC_RAM_SVOP_PDP to 0x2. */
    APB_MISC_GP_ASDBGREG_0 = (APB_MISC_GP_ASDBGREG_0 & 0xFCFFFFFF) | 0x02000000;
}