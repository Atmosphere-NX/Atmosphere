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

#include "breakpoints.h"
#include "utils.h"
#include "sysreg.h"
#include "arm.h"

void enableAndResetBreakpoints(void)
{
    SET_SYSREG(mdscr_el1, GET_SYSREG(mdscr_el1) | MDSCR_EL1_MDE);

    // 20 for watchpoints
    size_t num = ((GET_SYSREG(id_aa64dfr0_el1) >> 12) & 0xF) + 1;
    initBreakpointRegs(num);
}
