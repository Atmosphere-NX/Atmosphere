/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include "defines.hpp"

namespace ams::hvisor {

    struct GicV2Distributor {
        static constexpr u32 maxIrqId = 1019;
        static constexpr u32 spuriousGrpNeedAckIrqId = 1022;
        static constexpr u32 spuriousIrqId = 1023;

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
        u8 cpendsgir[16];
        u8 spendsgir[16];
        u8 _0xf30[0xFE8 - 0xF30];
        u32 icpidr2;
        u8 _0xfec[0x1000 - 0xFEC];

        enum SgirTargetListFilter : u32 {
            ForwardToTargetList = 0,
            ForwardToAllOthers  = 1,
            ForwardToSelf       = 2,
        };

        static constexpr int GetCfgrShift(u32 id) {
            return 2 * (id % 16);
        }
    };

    struct GicV2Controller {
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
    };

    // GICH
    struct GicV2VirtualInterfaceController {
        union HypervisorControlRegister {
            struct {
                u32 en         : 1;
                u32 uie        : 1;
                u32 lrenpie    : 1;
                u32 npie       : 1;
                u32 vgrp0eie   : 1;
                u32 vgrp0die   : 1;
                u32 vgrp1eie   : 1;
                u32 vgrp1die   : 1;
                u32 _8         : 19;
                u32 eoiCount   : 5;
            };
            u32 raw;
        };

        union VmControlRegister {
            struct {
                u32 enableGrp0      : 1;
                u32 enableGrp1      : 1;
                u32 ackCtl          : 1;
                u32 fiqEn           : 1;
                u32 cbpr            : 1;
                u32 _5              : 4;
                u32 eoiMode         : 1;
                u32 _10             : 8;
                u32 abpr            : 3;
                u32 bpr             : 3;
                u32 _24             : 3;
                u32 pmr             : 5;
            };
            u32 raw;
        };

        union MaintenanceIntStatRegister {
            struct {
                u32 eoi     : 1;
                u32 u       : 1;
                u32 lrenp   : 1;
                u32 np      : 1;
                u32 vgrp0e  : 1;
                u32 vgrp0d  : 1;
                u32 vgrp1e  : 1;
                u32 vgrp1d  : 1;
                u32 _8      : 24;
            };
            u32 raw;
        };

        union ListRegister {
            struct {
                u32 virtualId       : 10;
                u32 physicalId      : 10; // note: different encoding if hw = 0 (can't represent it in struct)
                u32 sbz2            : 3;
                u32 priority        : 5;
                u32 pending         : 1;
                u32 active          : 1;
                u32 grp1            : 1;
                u32 hw              : 1;
            };
            u32 raw;
        };

        HypervisorControlRegister hcr;
        u32 vtr;
        VmControlRegister vmcr;
        u8 _0x0c[0x10 - 0xC];
        MaintenanceIntStatRegister misr;
        u8 _0x14[0x20 - 0x14];
        u32 eisr0;
        u32 eisr1;
        u8 _0x28[0x30 - 0x28];
        u32 elsr0;
        u32 elsr1;
        u8 _0x38[0xF0 - 0x38];
        u32 apr;
        u8 _0xf4[0x100 - 0xF4];
        ListRegister lr[64];
    };

    struct GicV2VirtualInterface : public GicV2Controller {
        // Allowed because no non-static members
        static constexpr u32 numPriorityLevels = 32;
        static constexpr u8  idlePriorityLevel = 0xF8;
    };

    static_assert(std::is_standard_layout_v<GicV2Distributor>);
    static_assert(std::is_standard_layout_v<GicV2Controller>);
    static_assert(std::is_standard_layout_v<GicV2VirtualInterfaceController>);
    static_assert(std::is_standard_layout_v<GicV2VirtualInterface>);

    static_assert(std::is_trivial_v<GicV2Distributor>);
    static_assert(std::is_trivial_v<GicV2Controller>);
    static_assert(std::is_trivial_v<GicV2VirtualInterfaceController>);
    static_assert(std::is_trivial_v<GicV2VirtualInterface>);
}
