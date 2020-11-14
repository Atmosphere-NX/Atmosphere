/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License
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
#pragma once

#define ICTLR_REG_BASE(irq) ((((irq) - 32) >> 5) * 0x100)
#define ICTLR_FIR_SET(irq)  (ICTLR_REG_BASE(irq) + 0x18)
#define ICTLR_FIR_CLR(irq)  (ICTLR_REG_BASE(irq) + 0x1c)
#define FIR_BIT(irq)        (1 << ((irq) & 0x1f))

#define INT_GIC_BASE            (0)
#define INT_PRI_BASE            (INT_GIC_BASE + 32)
#define INT_SHR_SEM_OUTBOX_IBF  (INT_PRI_BASE + 6)
