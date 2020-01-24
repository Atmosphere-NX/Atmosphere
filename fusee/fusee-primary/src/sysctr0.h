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
 
#ifndef FUSEE_SYSCTR0_H
#define FUSEE_SYSCTR0_H

#include <stdint.h>

#define SYSCTR0_BASE 0x700F0000
#define MAKE_SYSCTR0_REG(n) MAKE_REG32(SYSCTR0_BASE + n)

#define SYSCTR0_CNTCR_0 MAKE_SYSCTR0_REG(0x00)
#define SYSCTR0_CNTSR_0 MAKE_SYSCTR0_REG(0x04)
#define SYSCTR0_CNTCV0_0 MAKE_SYSCTR0_REG(0x08)
#define SYSCTR0_CNTCV1_0 MAKE_SYSCTR0_REG(0x0C)
#define SYSCTR0_CNTFID0_0 MAKE_SYSCTR0_REG(0x20)
#define SYSCTR0_CNTFID1_0 MAKE_SYSCTR0_REG(0x24)
#define SYSCTR0_COUNTERID4_0 MAKE_SYSCTR0_REG(0xFD0)
#define SYSCTR0_COUNTERID5_0 MAKE_SYSCTR0_REG(0xFD4)
#define SYSCTR0_COUNTERID6_0 MAKE_SYSCTR0_REG(0xFD8)
#define SYSCTR0_COUNTERID7_0 MAKE_SYSCTR0_REG(0xFDC)
#define SYSCTR0_COUNTERID0_0 MAKE_SYSCTR0_REG(0xFE0)
#define SYSCTR0_COUNTERID1_0 MAKE_SYSCTR0_REG(0xFE4)
#define SYSCTR0_COUNTERID2_0 MAKE_SYSCTR0_REG(0xFE8)
#define SYSCTR0_COUNTERID3_0 MAKE_SYSCTR0_REG(0xFEC)
#define SYSCTR0_COUNTERID8_0 MAKE_SYSCTR0_REG(0xFF0)
#define SYSCTR0_COUNTERID9_0 MAKE_SYSCTR0_REG(0xFF4)
#define SYSCTR0_COUNTERID10_0 MAKE_SYSCTR0_REG(0xFF8)
#define SYSCTR0_COUNTERID11_0 MAKE_SYSCTR0_REG(0xFFC)

#endif
