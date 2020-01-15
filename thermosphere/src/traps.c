/*
 * Copyright (c) 2019 Atmosphère-NX
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

#include "traps.h"
#include "sysreg.h"

static inline void enableDebugTraps(void)
{
    u64 mdcr = GET_SYSREG(mdcr_el2);

    // Trap Debug Exceptions, and accesses to debug registers.
    mdcr |= MDCR_EL2_TDE;

    // Implied from TDE
    mdcr |= MDCR_EL2_TDRA | MDCR_EL2_TDOSA | MDCR_EL2_TDA;

    SET_SYSREG(mdcr_el2, mdcr);
}

static inline void enableTimerTraps(void)
{
    // Disable event streams, trap everything
    u64 cnthctl = 0;
    SET_SYSREG(cnthctl_el2, cnthctl);
}

void enableTraps(void)
{
    u64 hcr = GET_SYSREG(hcr_el2);

    // Trap SMC instructions
    hcr |= HCR_TSC;

    // Trap set/way isns
    hcr |= HCR_TSW;

    // Reroute physical IRQs to EL2
    hcr |= HCR_IMO;

    // Make sure HVC is enabled
    hcr &= ~HCR_HCD;

    SET_SYSREG(hcr_el2, hcr);

    enableDebugTraps();
    enableTimerTraps();
}
