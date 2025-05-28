/*
 * Copyright (c) Atmosphère-NX
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

namespace ams::kern::arch::arm {

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
        union {
            u8  bytes[1020];
            u32 words[255];
        } ipriorityr;
        u32 _0x7fc;
        union {
            u8  bytes[1020];
            u32 words[255];
        } itargetsr;
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

        static constexpr size_t SgirCpuTargetListShift = 16;

        enum SgirTargetListFilter : u32 {
            SgirTargetListFilter_CpuTargetList = (0 << 24),
            SgirTargetListFilter_Others        = (1 << 24),
            SgirTargetListFilter_Self          = (2 << 24),
            SgirTargetListFilter_Reserved      = (3 << 24),
        };
    };
    static_assert(util::is_pod<GicDistributor>::value);
    static_assert(sizeof(GicDistributor) == 0x1000);

    struct GicCpuInterface {
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
    static_assert(util::is_pod<GicCpuInterface>::value);
    static_assert(sizeof(GicCpuInterface) == 0x2000);

    struct KInterruptController {
        NON_COPYABLE(KInterruptController);
        NON_MOVEABLE(KInterruptController);
        public:
            static constexpr s32 NumSoftwareInterrupts = 16;
            static constexpr s32 NumLocalInterrupts = NumSoftwareInterrupts + 16;
            static constexpr s32 NumGlobalInterrupts = 988;
            static constexpr s32 NumInterrupts = NumLocalInterrupts + NumGlobalInterrupts;
            static constexpr s32 NumPriorityLevels = 4;
        public:
            struct LocalState {
                u32 isenabler[NumLocalInterrupts / 32];
                u32 ipriorityr[NumLocalInterrupts / 4];
                u32 itargetsr[NumLocalInterrupts / 4];
                u32 icfgr[NumLocalInterrupts / 16];
                u32 spendsgir[4];
            };
            static_assert(sizeof(LocalState{}.spendsgir) == sizeof(GicDistributor{}.spendsgir));

            struct GlobalState {
                u32 isenabler[NumGlobalInterrupts / 32];
                u32 ipriorityr[NumGlobalInterrupts / 4];
                u32 itargetsr[NumGlobalInterrupts / 4];
                u32 icfgr[NumGlobalInterrupts / 16];
            };

            enum PriorityLevel : u8 {
                PriorityLevel_High  = 0,
                PriorityLevel_Low   = NumPriorityLevels - 1,

                PriorityLevel_Timer     = 1,
                PriorityLevel_Scheduler = 2,
            };
        private:
            static constinit inline u32 s_mask[cpu::NumCores];
        private:
            volatile GicDistributor  *m_gicd;
            volatile GicCpuInterface *m_gicc;
        public:
            constexpr KInterruptController() : m_gicd(nullptr), m_gicc(nullptr) { /* ... */ }

            void Initialize(s32 core_id);
            void Finalize(s32 core_id);

            void SaveCoreLocal(LocalState *state) const;
            void SaveGlobal(GlobalState *state) const;
            void RestoreCoreLocal(const LocalState *state) const;
            void RestoreGlobal(const GlobalState *state) const;
        public:
            u32 GetIrq() const {
                return m_gicc->iar;
            }

            static constexpr s32 ConvertRawIrq(u32 irq) {
                return (irq == 0x3FF) ? -1 : (irq & 0x3FF);
            }

            void Enable(s32 irq) const {
                m_gicd->isenabler[irq / BITSIZEOF(u32)] = (1u << (irq % BITSIZEOF(u32)));
            }

            void Disable(s32 irq) const {
                m_gicd->icenabler[irq / BITSIZEOF(u32)] = (1u << (irq % BITSIZEOF(u32)));
            }

            void Clear(s32 irq) const {
                m_gicd->icpendr[irq / BITSIZEOF(u32)] = (1u << (irq % BITSIZEOF(u32)));
            }

            void SetTarget(s32 irq, s32 core_id) const {
                m_gicd->itargetsr.bytes[irq] = m_gicd->itargetsr.bytes[irq] | GetGicMask(core_id);
            }

            void ClearTarget(s32 irq, s32 core_id) const {
                m_gicd->itargetsr.bytes[irq] = m_gicd->itargetsr.bytes[irq] & ~GetGicMask(core_id);
            }

            void SetPriorityLevel(s32 irq, s32 level) const {
                MESOSPHERE_ASSERT(PriorityLevel_High <= level && level <= PriorityLevel_Low);
                m_gicd->ipriorityr.bytes[irq] = ToGicPriorityValue(level);
            }

            s32 GetPriorityLevel(s32 irq) const {
                return FromGicPriorityValue(m_gicd->ipriorityr.bytes[irq]);
            }

            void SetPriorityLevel(s32 level) const {
                MESOSPHERE_ASSERT(PriorityLevel_High <= level && level <= PriorityLevel_Low);
                m_gicc->pmr = ToGicPriorityValue(level);
            }

            void SetEdge(s32 irq) const {
                u32 cfg = m_gicd->icfgr[irq / (BITSIZEOF(u32) / 2)];
                cfg &= ~(0x3 << (2 * (irq % (BITSIZEOF(u32) / 2))));
                cfg |=  (0x2 << (2 * (irq % (BITSIZEOF(u32) / 2))));
                m_gicd->icfgr[irq / (BITSIZEOF(u32) / 2)] = cfg;
            }

            void SetLevel(s32 irq) const {
                u32 cfg = m_gicd->icfgr[irq / (BITSIZEOF(u32) / 2)];
                cfg &= ~(0x3 << (2 * (irq % (BITSIZEOF(u32) / 2))));
                cfg |=  (0x0 << (2 * (irq % (BITSIZEOF(u32) / 2))));
                m_gicd->icfgr[irq / (BITSIZEOF(u32) / 2)] = cfg;
            }

            void SendInterProcessorInterrupt(s32 irq, u64 core_mask) {
                MESOSPHERE_ASSERT(IsSoftware(irq));
                m_gicd->sgir = GetCpuTargetListMask(irq, core_mask);
            }

            void SendInterProcessorInterrupt(s32 irq) {
                MESOSPHERE_ASSERT(IsSoftware(irq));
                m_gicd->sgir = GicDistributor::SgirTargetListFilter_Others | irq;
            }

            void EndOfInterrupt(u32 irq) const {
                m_gicc->eoir = irq;
            }

            bool IsInterruptDefined(s32 irq) const {
                const s32 num_interrupts = std::min(32 + 32 * (m_gicd->typer & 0x1F), static_cast<u32>(NumInterrupts));
                return (0 <= irq && irq < num_interrupts);
            }
        public:
            static constexpr ALWAYS_INLINE bool IsSoftware(s32 id) {
                MESOSPHERE_ASSERT(0 <= id && id < NumInterrupts);
                return id < NumSoftwareInterrupts;
            }

            static constexpr ALWAYS_INLINE bool IsLocal(s32 id) {
                MESOSPHERE_ASSERT(0 <= id && id < NumInterrupts);
                return id < NumLocalInterrupts;
            }

            static constexpr ALWAYS_INLINE bool IsGlobal(s32 id) {
                MESOSPHERE_ASSERT(0 <= id && id < NumInterrupts);
                return NumLocalInterrupts <= id;
            }

            static constexpr size_t GetGlobalInterruptIndex(s32 id) {
                MESOSPHERE_ASSERT(IsGlobal(id));
                return id - NumLocalInterrupts;
            }

            static constexpr size_t GetLocalInterruptIndex(s32 id) {
                MESOSPHERE_ASSERT(IsLocal(id));
                return id;
            }
        private:
            static constexpr size_t PriorityShift = BITSIZEOF(u8) - util::CountTrailingZeros(NumPriorityLevels);
            static_assert(PriorityShift < BITSIZEOF(u8));
            static_assert(util::IsPowerOfTwo(NumPriorityLevels));

            static constexpr ALWAYS_INLINE u8 ToGicPriorityValue(s32 level) {
                return (level << PriorityShift) | ((1 << PriorityShift) - 1);
            }

            static constexpr ALWAYS_INLINE s32 FromGicPriorityValue(u8 priority) {
                return (priority >> PriorityShift) & (NumPriorityLevels - 1);
            }

            static constexpr ALWAYS_INLINE s32 GetCpuTargetListMask(s32 irq, u64 core_mask) {
                MESOSPHERE_ASSERT(IsSoftware(irq));
                MESOSPHERE_ASSERT(core_mask < (1ul << cpu::NumCores));
                return GicDistributor::SgirTargetListFilter_CpuTargetList | irq | (static_cast<u16>(core_mask) << GicDistributor::SgirCpuTargetListShift);
            }

            static ALWAYS_INLINE s32 GetGicMask(s32 core_id) {
                return s_mask[core_id];
            }

            ALWAYS_INLINE void SetGicMask(s32 core_id) const {
                s_mask[core_id] = m_gicd->itargetsr.bytes[0];
            }

            NOINLINE void SetupInterruptLines(s32 core_id) const;
    };

}
