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

#pragma once

#include "../../gicv2.h"

#define MEMORY_MAP_PA_GICD              0x50041000ull
#define MEMORY_MAP_PA_GICC              0x50042000ull
#define MEMORY_MAP_PA_GICH              0x50044000ull
#define MEMORY_MAP_PA_GICV              0x50046000ull

#define GIC_IRQID_MAINTENANCE           25
#define GIC_IRQID_NS_PHYS_HYP_TIMER     26
#define GIC_IRQID_NS_VIRT_TIMER         27
#define GIC_IRQID_LEGACY_NFIQ           28
#define GIC_IRQID_SEC_PHYS_TIMER        29
#define GIC_IRQID_NS_PHYS_TIMER         30
#define GIC_IRQID_LEGACY_NIRQ           31

#define GIC_IRQID_NS_VIRT_HYP_TIMER     GIC_IRQID_SPURIOUS // SBSA: 28. Unimplemented
#define GIC_IRQID_SEC_PHYS_HYP_TIMER    GIC_IRQID_SPURIOUS // SBSA: 20. Unimplemented
#define GIC_IRQID_SEC_VIRT_HYP_TIMER    GIC_IRQID_SPURIOUS // SBSA: 19. Unimplemented

#define GIC_IRQID_UARTA                 (32 + 36)
#define GIC_IRQID_UARTB                 (32 + 37)
#define GIC_IRQID_UARTC                 (32 + 46)
#define GIC_IRQID_UARTD                 (32 + 90)
