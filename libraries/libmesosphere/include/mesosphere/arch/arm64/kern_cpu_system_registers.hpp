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

namespace ams::kern::arm64::cpu {

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

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(SctlrEl1, sctlr_el1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(CpuActlrEl1, s3_1_c15_c2_0)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(CpuEctlrEl1, s3_1_c15_c2_1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(CsselrEl1, csselr_el1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(OslarEl1, oslar_el1)

    #define FOR_I_IN_0_TO_15(HANDLER, ...)                                                                              \
        HANDLER(0,  ## __VA_ARGS__) HANDLER(1,  ## __VA_ARGS__) HANDLER(2,  ## __VA_ARGS__) HANDLER(3,  ## __VA_ARGS__) \
        HANDLER(4,  ## __VA_ARGS__) HANDLER(5,  ## __VA_ARGS__) HANDLER(6,  ## __VA_ARGS__) HANDLER(7,  ## __VA_ARGS__) \
        HANDLER(8,  ## __VA_ARGS__) HANDLER(9,  ## __VA_ARGS__) HANDLER(10, ## __VA_ARGS__) HANDLER(11, ## __VA_ARGS__) \
        HANDLER(12, ## __VA_ARGS__) HANDLER(13, ## __VA_ARGS__) HANDLER(14, ## __VA_ARGS__) HANDLER(15, ## __VA_ARGS__) \

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
            u64 value;
        public:
            constexpr ALWAYS_INLINE GenericRegisterAccessorBase(u64 v) : value(v) { /* ... */ }
        protected:
            constexpr ALWAYS_INLINE u64 GetValue() const {
                return this->value;
            }

            constexpr ALWAYS_INLINE u64 GetBits(size_t offset, size_t count) const {
                return (this->value >> offset) & ((1ul << count) - 1);
            }

            constexpr ALWAYS_INLINE void SetBits(size_t offset, size_t count, u64 value) {
                const u64 mask = ((1ul << count) - 1) << offset;
                this->value &= ~mask;
                this->value |= (value & mask) << offset;
            }

            constexpr ALWAYS_INLINE void SetBit(size_t offset, bool enabled) {
                const u64 mask = 1ul << offset;
                if (enabled) {
                    this->value |= mask;
                } else {
                    this->value &= ~mask;
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

    #undef  FOR_I_IN_0_TO_15
    #undef  MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS_FUNCTIONS
    #undef  MESOSPHERE_CPU_SYSREG_ACCESSOR_CLASS
    #undef  MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS
    #undef  MESOSPHERE_CPU_GET_SYSREG
    #undef  MESOSPHERE_CPU_SET_SYSREG

}
