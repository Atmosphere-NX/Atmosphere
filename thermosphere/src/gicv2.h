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

#include "types.h"

#define GIC_IRQID_MAX                   1020
#define GIC_IRQID_SPURIOUS_GRPNEEDACK   (GIC_IRQID_MAX + 2)
#define GIC_IRQID_SPURIOUS              (GIC_IRQID_MAX + 3)

typedef struct ArmGicV2Distributor {
    u32 ctlr;
    u32 typer;
    u32 iidr;
    u8 _0x0c[0x80 - 0x0C];
    // Note: in reality only 512 interrupts max. are defined (nor "reserved") on Gicv2
    u32 igroupr[1024 / 32];
    u32 isenabler[1024 / 32];
    u32 icenabler[1024 / 32];
    u32 ispendr[1024 / 32];
    u32 icpendr[1024 / 32];
    u32 isactiver[1024 / 32];
    u32 icactiver[1024 / 32];
    u8 ipriorityr[1024]; // can be accessed as u8 or u32
    u8 itargetsr[1024]; // can be accessed as u8 or u32
    u32 icfgr[1024 / 16];
    u8 impldef_d00[0xF00 - 0xD00];
    u32 sgir;
    u8 _0xf04[0xF10 - 0xF04];
    u32 cpendsgir[4];
    u32 spendsgir[4];
    u8 _0xf30[0x1000 - 0xF30];
} ArmGicV2Distributor;

typedef struct ArmGicV2Controller {
    u32 ctlr;
    u32 pmr;
    u32 bpr;
    u32 iar;
    u32 eoir;
    u32 rpr;
    u32 hppir;
    u32 abpr;
    u32 aiar;
    u32 aeoir;
    u32 ahppir;
    u8 _0x2c[0x40 - 0x2C];
    u8 impldef_40[0xD0 - 0x40];
    u32 apr[4];
    u32 nsapr[4];
    u8 _0xf0[0xFC - 0xF0];
    u32 iidr;
    u8 _0x100[0x1000 - 0x100];
    u32 dir;
    u8 _0x1004[0x2000 - 0x1004];
} ArmGicV2Controller;

typedef struct ArmGicV2VirtualInterfaceController {
    u32 hcr;
    u32 vtr;
    u32 vmcr;
    u8 _0x0c[0x10 - 0xC];
    u32 misr;
    u8 _0x14[0x20 - 0x14];
    u32 eisr0;
    u32 eisr1;
    u8 _0x28[0x30 - 0x28];
    u32 elsr0;
    u32 elsr1;
    u8 _0x38[0xF0 - 0x38];
    u32 apr;
    u8 _0xf4[0x100 - 0xF4];
    u32 lr[64];
} ArmGicV2VirtualInterfaceController;


typedef struct ArmGicV2 {
    volatile ArmGicV2Distributor *gicd;
    volatile ArmGicV2Controller *gicc;
    volatile ArmGicV2VirtualInterfaceController *gich;
    volatile ArmGicV2Controller *gicv;
} ArmGicV2;
