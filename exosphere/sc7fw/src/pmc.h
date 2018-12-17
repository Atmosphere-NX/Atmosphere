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
 
#ifndef EXOSPHERE_BPMPFW_PMC_H
#define EXOSPHERE_BPMPFW_PMC_H

#include "utils.h"

#define PMC_BASE (0x7000E400)

#define MAKE_PMC_REG(ofs) (MAKE_REG32(PMC_BASE + ofs))

#define APBDEV_PMC_CNTRL_0           MAKE_PMC_REG(0x000)

#define APBDEV_PMC_DPD_SAMPLE_0      MAKE_PMC_REG(0x020)

#define APBDEV_PMC_DPD_ENABLE_0      MAKE_PMC_REG(0x024)

#define APBDEV_PMC_CLAMP_STATUS_0    MAKE_PMC_REG(0x02C)

#define APBDEV_PMC_PWRGATE_STATUS_0  MAKE_PMC_REG(0x038)

#define APBDEV_PMC_SCRATCH12_0       MAKE_PMC_REG(0x080)
#define APBDEV_PMC_SCRATCH13_0       MAKE_PMC_REG(0x084)
#define APBDEV_PMC_SCRATCH18_0       MAKE_PMC_REG(0x098)


#define APBDEV_PMC_WEAK_BIAS_0       MAKE_PMC_REG(0x2C8)

#define APBDEV_PMC_IO_DPD3_REQ_0     MAKE_PMC_REG(0x45C)
#define APBDEV_PMC_IO_DPD3_STATUS_0  MAKE_PMC_REG(0x460)

#define APBDEV_PMC_IO_DPD4_REQ_0     MAKE_PMC_REG(0x464)
#define APBDEV_PMC_IO_DPD4_STATUS_0  MAKE_PMC_REG(0x468)

#define APBDEV_PMC_SET_SW_CLAMP_0    MAKE_PMC_REG(0x47C)

#define APBDEV_PMC_DDR_CNTRL_0       MAKE_PMC_REG(0x4E4)

#endif
