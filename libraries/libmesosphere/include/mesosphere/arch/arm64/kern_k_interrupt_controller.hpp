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
#pragma once
#include <vapours.hpp>
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_typed_address.hpp>

namespace ams::kern::arm64 {

    struct GicDistributor {
        u32 ctlr;
        u32 typer;
        u32 iidr;
        u32 reserved_0x0c;
        u32 statusr;
        u32 reserved_0x14[3];
        u32 impldef_0x20[8];
        u32 setspi_nsr;
        u32 reserved_0x44;
        u32 clrspi_nsr;
        u32 reserved_0x4c;
        u32 setspi_sr;
        u32 reserved_0x54;
        u32 clrspi_sr;
        u32 reserved_0x5c[9];
        u32 igroupr[32];
        u32 isenabler[32];
        u32 icenabler[32];
        u32 ispendr[32];
        u32 icpendr[32];
        u32 isactiver[32];
        u32 icactiver[32];
        u8 ipriorityr[1020];
        u32 _0x7fc;
        u8 itargetsr[1020];
        u32 _0xbfc;
        u32 icfgr[64];
        u32 igrpmodr[32];
        u32 _0xd80[32];
        u32 nsacr[64];
        u32 sgir;
        u32 _0xf04[3];
        u32 cpendsgir[4];
        u32 spendsgir[4];
        u32 reserved_0xf30[52];
    };
    static_assert(std::is_pod<GicDistributor>::value);
    static_assert(sizeof(GicDistributor) == 0x1000);

    struct GicController {
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
        u32 statusr;
        u32 reserved_30[4];
        u32 impldef_40[36];
        u32 apr[4];
        u32 nsapr[4];
        u32 reserved_f0[3];
        u32 iidr;
        u32 reserved_100[960];
        u32 dir;
        u32 _0x1004[1023];
    };
    static_assert(std::is_pod<GicController>::value);
    static_assert(sizeof(GicController) == 0x2000);

    struct KInterruptController {
        NON_COPYABLE(KInterruptController);
        NON_MOVEABLE(KInterruptController);
        public:
            static constexpr size_t NumLocalInterrupts = 32;
            static constexpr size_t NumGlobalInterrupts = 988;
            static constexpr size_t NumInterrupts = NumLocalInterrupts + NumGlobalInterrupts;
        public:
            struct LocalState {
                u32 local_isenabler[NumLocalInterrupts / 32];
                u32 local_ipriorityr[NumLocalInterrupts / 4];
                u32 local_targetsr[NumLocalInterrupts / 4];
                u32 local_icfgr[NumLocalInterrupts / 16];
            };

            struct GlobalState {
                u32 global_isenabler[NumGlobalInterrupts / 32];
                u32 global_ipriorityr[NumGlobalInterrupts / 4];
                u32 global_targetsr[NumGlobalInterrupts / 4];
                u32 global_icfgr[NumGlobalInterrupts / 16];
            };
        private:
            static inline volatile GicDistributor *s_gicd;
            static inline volatile GicController  *s_gicc;
            static inline u32 s_mask[cpu::NumCores];
        private:
            volatile GicDistributor *gicd;
            volatile GicController  *gicc;
        public:
            KInterruptController() { /* Don't initialize anything -- this will be taken care of by ::Initialize() */ }

            /* TODO: Actually implement KInterruptController functionality. */
    };
}
