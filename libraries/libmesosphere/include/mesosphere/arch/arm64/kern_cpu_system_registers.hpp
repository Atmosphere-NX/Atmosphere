/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(MairEl1, mair_el1)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(TcrEl1, tcr_el1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(SctlrEl1, sctlr_el1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(CpuActlrEl1, s3_1_c15_c2_0)
    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(CpuEctlrEl1, s3_1_c15_c2_1)

    MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS(CsselrEl1, csselr_el1)

    /* Base class for register accessors. */
    class GenericRegisterAccessor {
        private:
            u64 value;
        public:
            ALWAYS_INLINE GenericRegisterAccessor(u64 v) : value(v) { /* ... */ }
        protected:
            constexpr ALWAYS_INLINE u64 GetBits(size_t offset, size_t count) const {
                return (this->value >> offset) & ((1 << count) - 1);
            }
    };

    /* Special code for main id register. */
    class MainIdRegisterAccessor : public GenericRegisterAccessor {
        public:
            enum class Implementer {
                ArmLimited = 0x41,
            };
            enum class PrimaryPartNumber {
                CortexA53  = 0xD03,
                CortexA57  = 0xD07,
            };
        public:
            ALWAYS_INLINE MainIdRegisterAccessor() : GenericRegisterAccessor(MESOSPHERE_CPU_GET_SYSREG(midr_el1)) { /* ... */ }
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
    class CacheLineIdAccessor : public GenericRegisterAccessor {
        public:
            ALWAYS_INLINE CacheLineIdAccessor() : GenericRegisterAccessor(MESOSPHERE_CPU_GET_SYSREG(clidr_el1)) { /* ... */ }
        public:
            constexpr ALWAYS_INLINE int GetLevelsOfCoherency() const {
                return static_cast<int>(this->GetBits(24, 3));
            }

            constexpr ALWAYS_INLINE int GetLevelsOfUnification() const {
                return static_cast<int>(this->GetBits(21, 3));
            }

            /* TODO: Other bitfield accessors? */
    };

    class CacheSizeIdAccessor : public GenericRegisterAccessor {
        public:
            ALWAYS_INLINE CacheSizeIdAccessor() : GenericRegisterAccessor(MESOSPHERE_CPU_GET_SYSREG(ccsidr_el1)) { /* ... */ }
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

    #undef  MESOSPHERE_CPU_DEFINE_SYSREG_ACCESSORS
    #undef  MESOSPHERE_CPU_GET_SYSREG
    #undef  MESOSPHERE_CPU_SET_SYSREG

}
