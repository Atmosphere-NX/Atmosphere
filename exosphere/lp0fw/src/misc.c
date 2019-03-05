/*
 * Copyright (c) 2018 Atmosphère-NX
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
    /* Enable RTCK daisychaining by setting TBE bit. */
    APB_MISC_PP_CONFIG_CTL_0 = 0x80;
    
    /* Literally none of this is documented in the TRM, lol. */
    if (FUSE_CHIP_REGS->FUSE_SECURITY_MODE == 1) {
        uint32_t secure_boot_val = 0b0100; /* Sets NIDEN for aarch64. */
        uint32_t misc_val = 0x40;
        if (APBDEV_PMC_STICKY_BITS_0 & 0x40) {
            misc_val = 0x0;
        } else {
            secure_boot_val = 0b1101; /* Sets SPNIDEN, NIDEN, DBGEN for aarch64. */
        }
        SB_PFCFG_0 = (SB_PFCFG_0 & ~0b1111) | secure_boot_val; /* Configures debug bits. */
        APB_MISC_PP_CONFIG_CTL_0 |= misc_val; /* Undocumented, seems to control invasive debugging/JTAG. */
    }
    
    /* Set sticky bits based SECURITY_MODE. */
    APBDEV_PMC_STICKY_BITS_0 |= FUSE_CHIP_REGS->FUSE_SECURITY_MODE;
    
    /* Set E_INPUT in PINMUX_AUX_GPIO_PA6_0 */
    PINMUX_AUX_GPIO_PA6_0 |= 0x40;
}

void misc_restore_ram_svop(void) {
    /* This sets CFG2TMC_RAM_SVOP_PDP to 0x2. */
    APB_MISC_GP_ASDBGREG_0 = (APB_MISC_GP_ASDBGREG_0 & 0xFCFFFFFF) | 0x02000000;
}