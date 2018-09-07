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
 
#ifndef EXOSPHERE_ACTIVITY_MONITOR_H
#define EXOSPHERE_ACTIVITY_MONITOR_H

#include "sysreg.h"

/* Exosphere Driver for the Tegra X1 Activity Monitor. */

/* NOTE: ACTMON registers lie in the SYSREG region! */
#define ACTMON_BASE (SYSREG_BASE + 0x800)

#define MAKE_ACTMON_REG(n) MAKE_REG32(ACTMON_BASE + n)

#define ACTMON_GLB_STATUS_0 MAKE_ACTMON_REG(0x000)
#define ACTMON_GLB_PERIOD_CTRL_0 MAKE_ACTMON_REG(0x004)
#define ACTMON_COP_CTRL_0 MAKE_ACTMON_REG(0x0C0)
#define ACTMON_COP_UPPER_WMARK_0 MAKE_ACTMON_REG(0x0C4)
#define ACTMON_COP_INTR_STATUS_0 MAKE_ACTMON_REG(0x0E4)

void actmon_interrupt_handler(void);

void actmon_on_bpmp_wakeup(void);

void actmon_set_callback(void (*callback)(void));

#endif
