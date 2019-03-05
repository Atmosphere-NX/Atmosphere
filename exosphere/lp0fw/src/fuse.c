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
#include "fuse.h"
#include "car.h"
#include "pmc.h"

#define NUM_FUSE_BYPASS_ENTRIES 0

bool fuse_check_downgrade_status(void) {
    /* We aren't going to implement anti-downgrade. */
    return false;
}

void fuse_disable_programming(void) {
    FUSE_REGS->FUSE_DIS_PGM = 1;
}

static fuse_bypass_data_t g_fuse_bypass_entries[NUM_FUSE_BYPASS_ENTRIES] = {
    /* No entries here. */
};

void fuse_configure_fuse_bypass(void) {
    /* Enable fuses in CAR? This seems to affect fuse data visibility. */
    CLK_RST_CONTROLLER_MISC_CLK_ENB_0 |= 0x10000000;
    
    /* Configure bypass/override, only if programming is allowed.  */
    if (!(FUSE_REGS->FUSE_DIS_PGM & 1)) {
        /* Enable write access. */
        FUSE_REGS->FUSE_WRITE_ACCESS = (FUSE_REGS->FUSE_WRITE_ACCESS & ~0x1) | 0x10000;
        /* Enable fuse bypass config. */
        FUSE_REGS->FUSE_FUSEBYPASS = 1;
        
        /* Override fuses. */
        for (size_t i = 0; i < NUM_FUSE_BYPASS_ENTRIES; i++) {
            MAKE_FUSE_REG(g_fuse_bypass_entries[i].offset) = g_fuse_bypass_entries[i].value;
        }
        
        /* Disable fuse write access. */
        FUSE_REGS->FUSE_WRITE_ACCESS |= 1;
        
        /* Enable fuse bypass config. */
        /* I think this is a bug, and Nintendo meant to write 0 here? */
        FUSE_REGS->FUSE_FUSEBYPASS = 1;
        
        /* This...clears the disable programming bit(?). */
        /* I have no idea why this happens. What? */
        /* This is probably also either a bug or does nothing. */
        /* Is this bit even clearable? */
        FUSE_REGS->FUSE_DIS_PGM &= 0xFFFFFFFE;
        
        /* Restore saved private key disable bit. */
        FUSE_REGS->FUSE_PRIVATEKEYDISABLE |= (APBDEV_PMC_SECURE_SCRATCH21_0 & 0x10);
        
        /* Lock private key disable secure scratch. */
        APBDEV_PMC_SEC_DISABLE2_0 |= 0x4000000;
    }
    
}
