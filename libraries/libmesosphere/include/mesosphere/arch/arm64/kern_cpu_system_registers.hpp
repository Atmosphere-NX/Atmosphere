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

namespace ams::kern::arch::arm64::cpu {

    #define MESOSPHERE_CPU_GET_SYSREG(name)                                            \
        ({                                                                             \
            u64 temp_value;                                                            \
            __asm__ __volatile__("mrs %0, " #name "" : "=&r"(temp_value) :: "memory"); \
            temp_value;                                                                \
        })

    #define MESOSPHERE_CPU_SET_SYSREG(name, value)                                    \
        ({                                                                            \
            __asm__ __volatile__("msr " #name ", %0" :: "r"(value) : "memory", "cc"); \
        })

    #define MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(name, reg_name)                          \
    ALWAYS_INLINE void Set##name(u64 value) { MESOSPHERE_CPU_SET_SYSREG(reg_name, value); } \
    ALWAYS_INLINE u64  Get##name()          { return MESOSPHERE_CPU_GET_SYSREG(reg_name); }

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(Ttbr0El1, ttbr0_el1)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(Ttbr1El1, ttbr1_el1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(TcrEl1, tcr_el1)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(MairEl1, mair_el1)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(TpidrEl1, tpidr_el1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(VbarEl1, vbar_el1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(FarEl1, far_el1)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(ParEl1, par_el1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(SctlrEl1, sctlr_el1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(CpuActlrEl1, s3_1_c15_c2_0)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(CpuEctlrEl1, s3_1_c15_c2_1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(CsselrEl1, csselr_el1)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(CcsidrEl1, ccsidr_el1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(OslarEl1, oslar_el1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(TpidrEl0,   tpidr_el0)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(TpidrRoEl0, tpidrro_el0)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(ElrEl1,   elr_el1)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(EsrEl1,   esr_el1)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(SpsrEl1,  spsr_el1)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(Afsr0El1, afsr0_el1)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(Afsr1El1, afsr1_el1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(MdscrEl1, mdscr_el1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(CpacrEl1, cpacr_el1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(ContextidrEl1, contextidr_el1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(CntkCtlEl1,  cntkctl_el1)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(CntpCtlEl0,  cntp_ctl_el0)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(CntpCvalEl0, cntp_cval_el0)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(Daif, daif)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(SpEl0, sp_el0)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(IdAa64Dfr0El1, id_aa64dfr0_el1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(PmcrEl0,       pmcr_el0)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(PmUserEnrEl0,  pmuserenr_el0)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(PmcCntrEl0,    pmccntr_el0)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(PmSelrEl0,     pmselr_el0)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(PmcCfiltrEl0,  pmccfiltr_el0)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(PmIntEnSetEl1, pmintenset_el1)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(PmCntEnSetEl0, pmcntenset_el0)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(PmOvsSetEl0,   pmovsset_el0)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(PmIntEnClrEl1, pmintenclr_el1)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(PmCntEnClrEl0, pmcntenclr_el0)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(PmOvsClrEl0,   pmovsclr_el0)

    #define FOR_I_IN_0_TO_30(HANDLER, ...)                                                                              \
        HANDLER(0,  ## __VA_ARGS__) HANDLER(1,  ## __VA_ARGS__) HANDLER(2,  ## __VA_ARGS__) HANDLER(3,  ## __VA_ARGS__) \
        HANDLER(4,  ## __VA_ARGS__) HANDLER(5,  ## __VA_ARGS__) HANDLER(6,  ## __VA_ARGS__) HANDLER(7,  ## __VA_ARGS__) \
        HANDLER(8,  ## __VA_ARGS__) HANDLER(9,  ## __VA_ARGS__) HANDLER(10, ## __VA_ARGS__) HANDLER(11, ## __VA_ARGS__) \
        HANDLER(12, ## __VA_ARGS__) HANDLER(13, ## __VA_ARGS__) HANDLER(14, ## __VA_ARGS__) HANDLER(15, ## __VA_ARGS__) \
        HANDLER(16, ## __VA_ARGS__) HANDLER(17, ## __VA_ARGS__) HANDLER(18, ## __VA_ARGS__) HANDLER(19, ## __VA_ARGS__) \
        HANDLER(20, ## __VA_ARGS__) HANDLER(21, ## __VA_ARGS__) HANDLER(22, ## __VA_ARGS__) HANDLER(23, ## __VA_ARGS__) \
        HANDLER(24, ## __VA_ARGS__) HANDLER(25, ## __VA_ARGS__) HANDLER(26, ## __VA_ARGS__) HANDLER(27, ## __VA_ARGS__) \
        HANDLER(28, ## __VA_ARGS__) HANDLER(29, ## __VA_ARGS__) HANDLER(30, ## __VA_ARGS__)

    #define MESOSPHERE_CPU_DEFINE_PMEV_ACCESSORS(ID, ...)                               \
        MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(PmevCntr##ID##El0,  pmevcntr##ID##_el0)  \
        MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(PmevTyper##ID##El0, pmevtyper##ID##_el0)

    FOR_I_IN_0_TO_30(MESOSPHERE_CPU_DEFINE_PMEV_ACCESSORS)

    #undef MESOSPHERE_CPU_DEFINE_PMEV_ACCESSORS
    #undef FOR_I_IN_0_TO_30

    #define FOR_I_IN_0_TO_15(HANDLER, ...)                                                                              \
        HANDLER(0,  ## __VA_ARGS__) HANDLER(1,  ## __VA_ARGS__) HANDLER(2,  ## __VA_ARGS__) HANDLER(3,  ## __VA_ARGS__) \
        HANDLER(4,  ## __VA_ARGS__) HANDLER(5,  ## __VA_ARGS__) HANDLER(6,  ## __VA_ARGS__) HANDLER(7,  ## __VA_ARGS__) \
        HANDLER(8,  ## __VA_ARGS__) HANDLER(9,  ## __VA_ARGS__) HANDLER(10, ## __VA_ARGS__) HANDLER(11, ## __VA_ARGS__) \
        HANDLER(12, ## __VA_ARGS__) HANDLER(13, ## __VA_ARGS__) HANDLER(14, ## __VA_ARGS__) HANDLER(15, ## __VA_ARGS__)

    #define MESOSPHERE_CPU_DEFINE_DBG_SYSREG_ACCESSORS(ID, ...)                     \
        MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(DbgWcr##ID##El1, dbgwcr##ID##_el1)   \
        MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(DbgWvr##ID##El1, dbgwvr##ID##_el1)   \
        MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(DbgBcr##ID##El1, dbgbcr##ID##_el1)   \
        MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(DbgBvr##ID##El1, dbgbvr##ID##_el1)

    FOR_I_IN_0_TO_15(MESOSPHERE_CPU_DEFINE_DBG_SYSREG_ACCESSORS)

    #undef MESOSPHERE_CPU_DEFINE_DBG_SYSREG_ACCESSORS

    /* Base class for register accessors. */
    class GenericRegisterAccessorBase {
        NON_COPYABLE(GenericRegisterAccessorBase);
        NON_MOVEABLE(GenericRegisterAccessorBase);
        private:
            u64 m_value;
        public:
            constexpr ALWAYS_INLINE GenericRegisterAccessorBase(u64 v) : m_value(v) { /* ... */ }
        protected:
            constexpr ALWAYS_INLINE u64 GetValue() const {
                return m_value;
            }

            constexpr ALWAYS_INLINE u64 GetBits(size_t offset, size_t count) const {
                return (m_value >> offset) & ((1ul << count) - 1);
            }

            constexpr ALWAYS_INLINE void SetBits(size_t offset, size_t count, u64 value) {
                const u64 mask = ((1ul << count) - 1) << offset;
                m_value &= ~mask;
                m_value |= (value & (mask >> offset)) << offset;
            }

            constexpr ALWAYS_INLINE void SetBitsDirect(size_t offset, size_t count, u64 value) {
                const u64 mask = ((1ul << count) - 1) << offset;
                m_value &= ~mask;
                m_value |= (value & mask);
            }

            constexpr ALWAYS_INLINE void SetBit(size_t offset, bool enabled) {
                const u64 mask = 1ul << offset;
                if (enabled) {
                    m_value |= mask;
                } else {
                    m_value &= ~mask;
                }
            }
    };

    template<typename Derived>
    class GenericRegisterAccessor : public GenericRegisterAccessorBase {
        public:
            constexpr ALWAYS_INLINE GenericRegisterAccessor(u64 v) : GenericRegisterAccessorBase(v) { /* ... */ }
        protected:
            ALWAYS_INLINE void Store() const {
                static_cast<const Derived *>(this)->Store();
            }
    };

    #define MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(name) class name##RegisterAccessor : public GenericRegisterAccessor<name##RegisterAccessor>

    #define MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(accessor, reg_name)                                                  \
        ALWAYS_INLINE accessor##RegisterAccessor() : GenericRegisterAccessor(MESOSPHERE_CPU_GET_SYSREG(reg_name)) { /* ... */ } \
        constexpr ALWAYS_INLINE accessor##RegisterAccessor(u64 v) : GenericRegisterAccessor(v) { /* ... */ }                    \
                                                                                                                                \
        ALWAYS_INLINE void Store() { const u64 v = this->GetValue(); MESOSPHERE_CPU_SET_SYSREG(reg_name, v); }

    /* Accessors. */
    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(MemoryAccessIndirection) {
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(MemoryAccessIndirection, mair_el1)
    };

    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(TranslationControl) {
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(TranslationControl, tcr_el1)

            constexpr ALWAYS_INLINE size_t GetT1Size() const {
                const size_t shift_value = this->GetBits(16, 6);
                return size_t(1) << (size_t(64) - shift_value);
            }

            constexpr ALWAYS_INLINE bool GetEpd0() const {
                return this->GetBits(7, 1) != 0;
            }

            constexpr ALWAYS_INLINE decltype(auto) SetEpd0(bool set) {
                this->SetBit(7, set);
                return *this;
            }
    };

    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(ArchitecturalFeatureAccessControl) {
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(ArchitecturalFeatureAccessControl, cpacr_el1)

            constexpr ALWAYS_INLINE decltype(auto) SetFpEnabled(bool en) {
                if (en) {
                    this->SetBits(20, 2, 0x3);
                } else {
                    this->SetBits(20, 2, 0x0);
                }
                return *this;
            }

            constexpr ALWAYS_INLINE bool IsFpEnabled() {
                return this->GetBits(20, 2) != 0;
            }
    };

    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(DebugFeature) {
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(DebugFeature, id_aa64dfr0_el1)

            constexpr ALWAYS_INLINE size_t GetNumWatchpoints() const {
                return this->GetBits(20, 4);
            }

            constexpr ALWAYS_INLINE size_t GetNumBreakpoints() const {
                return this->GetBits(12, 4);
            }

            constexpr ALWAYS_INLINE size_t GetNumContextAwareBreakpoints() const {
                return this->GetBits(28, 4);
            }
    };

    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(MonitorDebugSystemControl) {
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(MonitorDebugSystemControl, mdscr_el1)

            constexpr ALWAYS_INLINE bool GetMde() const {
                return this->GetBits(15, 1) != 0;
            }

            constexpr ALWAYS_INLINE size_t GetTdcc() const {
                return this->GetBits(12, 1) != 0;
            }

            constexpr ALWAYS_INLINE decltype(auto) SetMde(bool set) {
                this->SetBit(15, set);
                return *this;
            }

            constexpr ALWAYS_INLINE decltype(auto) SetTdcc(bool set) {
                this->SetBit(12, set);
                return *this;
            }
    };

    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(MultiprocessorAffinity) {
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(MultiprocessorAffinity, mpidr_el1)

            constexpr ALWAYS_INLINE u64 GetAff0() const {
                return this->GetBits(0, 8);
            }

            constexpr ALWAYS_INLINE u64 GetAff1() const {
                return this->GetBits(8, 8);
            }

            constexpr ALWAYS_INLINE u64 GetAff2() const {
                return this->GetBits(16, 8);
            }

            constexpr ALWAYS_INLINE u64 GetAff3() const {
                return this->GetBits(32, 8);
            }

            constexpr ALWAYS_INLINE u64 GetCpuOnArgument() const {
                constexpr u64 Mask = 0x000000FF00FFFF00ul;
                return this->GetValue() & Mask;
            }
    };

    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(ThreadId) {
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(ThreadId, tpidr_el1)
    };

    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(OsLockAccess) {
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(OsLockAccess, oslar_el1)
    };

    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(ContextId) {
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(ContextId, contextidr_el1)

            constexpr ALWAYS_INLINE decltype(auto) SetProcId(u32 proc_id) {
                this->SetBits(0, BITSIZEOF(proc_id), proc_id);
                return *this;
            }
    };

    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(MainId) {
        public:
            enum class Implementer {
                ArmLimited = 0x41,
            };
            enum class PrimaryPartNumber {
                CortexA53  = 0xD03,
                CortexA57  = 0xD07,
            };
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(MainId, midr_el1)
        public:
            constexpr ALWAYS_INLINE Implementer GetImplementer() const {
                return static_cast<Implementer>(this->GetBits(24, 8));
            }

            constexpr ALWAYS_INLINE u64 GetVariant() const {
                return this->GetBits(20, 4);
            }

            constexpr ALWAYS_INLINE u64 GetArchitecture() const {
                return this->GetBits(16, 4);
            }

            constexpr ALWAYS_INLINE PrimaryPartNumber GetPrimaryPartNumber() const {
                return static_cast<PrimaryPartNumber>(this->GetBits(4, 12));
            }

            constexpr ALWAYS_INLINE u64 GetRevision() const {
                return this->GetBits(0, 4);
            }
    };

    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(SystemControl) {
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(SystemControl, sctlr_el1)

            constexpr ALWAYS_INLINE decltype(auto) SetWxn(bool en) {
                this->SetBit(19, en);
                return *this;
            }
    };

    /* Accessors for timer registers. */
    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(CounterTimerKernelControl) {
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(CounterTimerKernelControl, cntkctl_el1)

            constexpr ALWAYS_INLINE decltype(auto) SetEl0PctEn(bool en) {
                this->SetBit(0, en);
                return *this;
            }
    };

    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(CounterTimerPhysicalTimerControl) {
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(CounterTimerPhysicalTimerControl, cntp_ctl_el0)

            constexpr ALWAYS_INLINE decltype(auto) SetEnable(bool en) {
                this->SetBit(0, en);
                return *this;
            }

            constexpr ALWAYS_INLINE decltype(auto) SetIMask(bool en) {
                this->SetBit(1, en);
                return *this;
            }
    };

    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(CounterTimerPhysicalTimerCompareValue) {
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(CounterTimerPhysicalTimerCompareValue, cntp_cval_el0)

            constexpr ALWAYS_INLINE u64 GetCompareValue() {
                return this->GetValue();
            }

            constexpr ALWAYS_INLINE decltype(auto) SetCompareValue(u64 value) {
                this->SetBits(0, BITSIZEOF(value), value);
                return *this;
            }
    };

    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(CounterTimerPhysicalCountValue) {
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(CounterTimerPhysicalCountValue, cntpct_el0)

            constexpr ALWAYS_INLINE u64 GetCount() {
                return this->GetValue();
            }
    };

    /* Accessors for cache registers. */
    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(CacheLineId) {
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(CacheLineId, clidr_el1)
        public:
            constexpr ALWAYS_INLINE int GetLevelsOfCoherency() const {
                return static_cast<int>(this->GetBits(24, 3));
            }

            constexpr ALWAYS_INLINE int GetLevelsOfUnification() const {
                return static_cast<int>(this->GetBits(21, 3));
            }

            /* TODO: Other bitfield accessors? */
    };

    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(CacheSizeId) {
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(CacheSizeId, ccsidr_el1)
        public:
            constexpr ALWAYS_INLINE int GetNumberOfSets() const {
                return static_cast<int>(this->GetBits(13, 15));
            }

            constexpr ALWAYS_INLINE int GetAssociativity() const {
                return static_cast<int>(this->GetBits(3, 10));
            }

            constexpr ALWAYS_INLINE int GetLineSize() const {
                return static_cast<int>(this->GetBits(0, 3));
            }

            /* TODO: Other bitfield accessors? */
    };

    MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS(PerformanceMonitorsControl) {
        public:
            MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS(PerformanceMonitorsControl, pmcr_el0)
        public:
            constexpr ALWAYS_INLINE u64 GetN() const {
                return this->GetBits(11, 5);
            }

            constexpr ALWAYS_INLINE decltype(auto) SetEventCounterReset(bool en) {
                this->SetBit(1, en);
                return *this;
            }

            constexpr ALWAYS_INLINE decltype(auto) SetCycleCounterReset(bool en) {
                this->SetBit(2, en);
                return *this;
            }

            /* TODO: Other bitfield accessors? */
    };

    #undef  FOR_I_IN_0_TO_15
    #undef  MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS
    #undef  MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS
    #undef  MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS
    #undef  MESOSPHERE_CPU_GET_SYSREG
    #undef  MESOSPHERE_CPU_SET_SYSREG

}
