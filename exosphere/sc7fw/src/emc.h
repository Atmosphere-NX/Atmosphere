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
 
#ifndef EXOSPHERE_BPMPFW_EMC_H
#define EXOSPHERE_BPMPFW_EMC_H

#include "utils.h"

#define EMC_BASE (0x7001B000)

#define EMC0_BASE (0x7001E000)
#define EMC1_BASE (0x7001F000)


#define MAKE_EMC_REG(ofs) (MAKE_REG32(EMC_BASE + ofs))

#define MAKE_EMC0_REG(ofs) (MAKE_REG32(EMC0_BASE + ofs))
#define MAKE_EMC1_REG(ofs) (MAKE_REG32(EMC1_BASE + ofs))

#define EMC_CFG_0                    MAKE_EMC_REG(0x00C)

#define EMC_ADR_CFG_0                MAKE_EMC_REG(0x10)

#define EMC_TIMING_CONTROL_0         MAKE_EMC_REG(0x028)

#define EMC_SELF_REF_0               MAKE_EMC_REG(0x0E0)

#define EMC_MRW_0                    MAKE_EMC_REG(0x0E8)

#define EMC_FBIO_CFG5_0              MAKE_EMC_REG(0x104)

#define EMC_MRW3_0                   MAKE_EMC_REG(0x138)

#define EMC_AUTO_CAL_CONFIG_0        MAKE_EMC_REG(0x2A4)

#define EMC_REQ_CTRL_0               MAKE_EMC_REG(0x2B0)

#define EMC_EMC_STATUS_0             MAKE_EMC_REG(0x2B4)
#define EMC0_EMC_STATUS_0            MAKE_EMC0_REG(0x2B4)
#define EMC1_EMC_STATUS_0            MAKE_EMC1_REG(0x2B4)

#define EMC_CFG_DIG_DLL_0            MAKE_EMC_REG(0x2BC)
#define EMC0_CFG_DIG_DLL_0           MAKE_EMC0_REG(0x2BC)
#define EMC1_CFG_DIG_DLL_0           MAKE_EMC1_REG(0x2BC)

#define EMC_ZCAL_INTERVAL_0          MAKE_EMC_REG(0x2E0)

#define EMC_PMC_SCRATCH3_0           MAKE_EMC_REG(0x448)

#define EMC_FBIO_CFG7_0              MAKE_EMC_REG(0x584)

void emc_put_dram_in_self_refresh_mode(void);

#endif
