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
#include <stratosphere.hpp>
#include "ldr_capabilities.hpp"

namespace ams::ldr {

    namespace {

        /* Types. */
        enum class CapabilityId {
            KernelFlags     = 3,
            SyscallMask     = 4,
            MapRange        = 6,
            MapPage         = 7,
            MapRegion       = 10,
            InterruptPair   = 11,
            ApplicationType = 13,
            KernelVersion   = 14,
            HandleTable     = 15,
            DebugFlags      = 16,

            Empty = 32,
        };

        template<size_t Index, size_t Count, typename T = u32>
        using CapabilityField = util::BitPack32::Field<Index, Count, T>;

        #define DEFINE_CAPABILITY_FIELD(name, prev, ...)                                               \
            using name = CapabilityField<prev::Next, __VA_ARGS__>;                                     \
            constexpr ALWAYS_INLINE typename name::Type Get##name() const { return this->Get<name>(); }

        constexpr ALWAYS_INLINE CapabilityId GetCapabilityId(util::BitPack32 cap) {
            return static_cast<CapabilityId>(util::CountTrailingZeros<u32>(~cap.value));
        }

        constexpr inline util::BitPack32 EmptyCapability = {~u32{}};
        static_assert(GetCapabilityId(EmptyCapability) == CapabilityId::Empty);

#define CAPABILITY_CLASS_NAME(id) Capability##id

#define DEFINE_CAPABILITY_CLASS(id, member_functions)                                                                           \
        class CAPABILITY_CLASS_NAME(id) {                                                                                       \
            public:                                                                                                             \
                static constexpr CapabilityId Id = CapabilityId::id;                                                            \
                using IdBits   = CapabilityField<0, static_cast<size_t>(Id) + 1>;                                               \
                static constexpr u32 IdBitsValue = (static_cast<u32>(1) << static_cast<size_t>(Id)) - 1;                        \
            private:                                                                                                            \
                util::BitPack32 m_value;                                                                                        \
            private:                                                                                                            \
                template<typename FieldType>                                                                                    \
                constexpr ALWAYS_INLINE typename FieldType::Type Get() const { return m_value.Get<FieldType>(); }               \
                template<typename FieldType>                                                                                    \
                constexpr ALWAYS_INLINE void Set(typename FieldType::Type fv) { m_value.Set<FieldType>(fv); }                   \
                constexpr ALWAYS_INLINE u32 GetValue() const { return m_value.value; }                                          \
            public:                                                                                                             \
                constexpr ALWAYS_INLINE CAPABILITY_CLASS_NAME(id)(util::BitPack32 v) : m_value{v} { /* ... */ }                 \
                                                                                                                                \
                static constexpr CAPABILITY_CLASS_NAME(id) Decode(util::BitPack32 v) { return CAPABILITY_CLASS_NAME(id)(v); }   \
                                                                                                                                \
                member_functions                                                                                                \
        };                                                                                                                      \
        static_assert(std::is_trivially_destructible<CAPABILITY_CLASS_NAME(id)>::value)

        /* Class definitions. */
        DEFINE_CAPABILITY_CLASS(KernelFlags,
            DEFINE_CAPABILITY_FIELD(MaximumThreadPriority, IdBits,                6);
            DEFINE_CAPABILITY_FIELD(MinimumThreadPriority, MaximumThreadPriority, 6);
            DEFINE_CAPABILITY_FIELD(MinimumCoreId,         MinimumThreadPriority, 8);
            DEFINE_CAPABILITY_FIELD(MaximumCoreId,         MinimumCoreId,         8);

            bool IsValid(const util::BitPack32 *kac, size_t kac_count) const {
                for (size_t i = 0; i < kac_count; i++) {
                    if (GetCapabilityId(kac[i]) == Id) {
                        const auto restriction = Decode(kac[i]);

                        if (this->GetMinimumThreadPriority() < restriction.GetMinimumThreadPriority() ||
                            this->GetMaximumThreadPriority() > restriction.GetMaximumThreadPriority() ||
                            this->GetMinimumThreadPriority() > this->GetMaximumThreadPriority()) {
                            return false;
                        }

                        if (this->GetMinimumCoreId() < restriction.GetMinimumCoreId() ||
                            this->GetMaximumCoreId() > restriction.GetMaximumCoreId() ||
                            this->GetMinimumCoreId() > this->GetMaximumCoreId()) {
                            return false;
                        }

                        return true;
                    }
                }
                return false;
            }
        );

        DEFINE_CAPABILITY_CLASS(SyscallMask,
            DEFINE_CAPABILITY_FIELD(Mask,  IdBits, 24);
            DEFINE_CAPABILITY_FIELD(Index, Mask,   3);

            bool IsValid(const util::BitPack32 *kac, size_t kac_count) const {
                for (size_t i = 0; i < kac_count; i++) {
                    if (GetCapabilityId(kac[i]) == Id) {
                        const auto restriction = Decode(kac[i]);

                        if (this->GetIndex() == restriction.GetIndex() && this->GetMask() == restriction.GetMask()) {
                            return true;
                        }
                    }
                }
                return false;
            }
        );

        DEFINE_CAPABILITY_CLASS(MapRange,
            DEFINE_CAPABILITY_FIELD(AddressSize,  IdBits,      24);
            DEFINE_CAPABILITY_FIELD(Flag,         AddressSize, 1, bool);
            static constexpr size_t SizeMax = 0x100000;

            bool IsValid(const util::BitPack32 next_cap, const util::BitPack32 *kac, size_t kac_count) const {
                if (GetCapabilityId(next_cap) != Id) {
                    return false;
                }

                const auto next = Decode(next_cap);
                const u32 start = this->GetAddressSize();
                const u32 size = next.GetAddressSize();
                const u32 end = start + size;
                if (size >= SizeMax) {
                    return false;
                }

                for (size_t i = 0; i < kac_count; i++) {
                    if (GetCapabilityId(kac[i]) == Id) {
                        const auto restriction = Decode(kac[i++]);
                        if (i >= kac_count || GetCapabilityId(kac[i]) != Id) {
                            return false;
                        }
                        const auto restriction_next = Decode(kac[i]);
                        const u32 restriction_start = restriction.GetAddressSize();
                        const u32 restriction_size = restriction_next.GetAddressSize();
                        const u32 restriction_end = restriction_start + restriction_size;

                        if (restriction_size >= SizeMax) {
                            continue;
                        }

                        if (this->GetFlag() == restriction.GetFlag() && next.GetFlag() == restriction_next.GetFlag()) {
                            if (restriction_start <= start && start <= restriction_end && end <= restriction_end) {
                                return true;
                            }
                        }
                    }
                }
                return false;
            }
        );

        DEFINE_CAPABILITY_CLASS(MapPage,
            DEFINE_CAPABILITY_FIELD(Address, IdBits, 24);

            bool IsValid(const util::BitPack32 *kac, size_t kac_count) const {
                for (size_t i = 0; i < kac_count; i++) {
                    if (GetCapabilityId(kac[i]) == Id) {
                        const auto restriction = Decode(kac[i]);

                        if (this->GetValue() == restriction.GetValue()) {
                            return true;
                        }
                    }
                }
                return false;
            }
        );

        enum class MemoryRegionType : u32 {
            NoMapping         = 0,
            KernelTraceBuffer = 1,
            OnMemoryBootImage = 2,
            DTB               = 3,
        };

        DEFINE_CAPABILITY_CLASS(MapRegion,
            DEFINE_CAPABILITY_FIELD(Region0,   IdBits,    6, MemoryRegionType);
            DEFINE_CAPABILITY_FIELD(ReadOnly0, Region0,   1, bool);
            DEFINE_CAPABILITY_FIELD(Region1,   ReadOnly0, 6, MemoryRegionType);
            DEFINE_CAPABILITY_FIELD(ReadOnly1, Region1,   1, bool);
            DEFINE_CAPABILITY_FIELD(Region2,   ReadOnly1, 6, MemoryRegionType);
            DEFINE_CAPABILITY_FIELD(ReadOnly2, Region2,   1, bool);

            static bool IsValidRegionType(const util::BitPack32 *kac, size_t kac_count, MemoryRegionType region_type, bool is_read_only) {
                if (region_type != MemoryRegionType::NoMapping) {
                    for (size_t i = 0; i < kac_count; i++) {
                        if (GetCapabilityId(kac[i]) == Id) {
                            const auto restriction = Decode(kac[i]);

                            if ((restriction.GetRegion0() == region_type && (is_read_only || !restriction.GetReadOnly0())) ||
                                (restriction.GetRegion1() == region_type && (is_read_only || !restriction.GetReadOnly1())) ||
                                (restriction.GetRegion2() == region_type && (is_read_only || !restriction.GetReadOnly2())))
                            {
                                return true;
                            }
                        }
                    }

                    return false;
                } else {
                    return true;
                }
            }

            bool IsValid(const util::BitPack32 *kac, size_t kac_count) const {
                return IsValidRegionType(kac, kac_count, this->GetRegion0(), this->GetReadOnly0()) &&
                       IsValidRegionType(kac, kac_count, this->GetRegion1(), this->GetReadOnly1()) &&
                       IsValidRegionType(kac, kac_count, this->GetRegion2(), this->GetReadOnly2());
            }
        );

        DEFINE_CAPABILITY_CLASS(InterruptPair,
            DEFINE_CAPABILITY_FIELD(InterruptId0, IdBits,       10);
            DEFINE_CAPABILITY_FIELD(InterruptId1, InterruptId0, 10);
            static constexpr u32 EmptyInterruptId = 0x3FF;

            bool IsSingleIdValid(const u32 id, const util::BitPack32 *kac, size_t kac_count) const {
                for (size_t i = 0; i < kac_count; i++) {
                    if (GetCapabilityId(kac[i]) == Id) {
                        const auto restriction = Decode(kac[i]);

                        if (restriction.GetInterruptId0() == EmptyInterruptId && restriction.GetInterruptId1() == EmptyInterruptId) {
                            return true;
                        }

                        if (restriction.GetInterruptId0() == id || restriction.GetInterruptId1() == id) {
                            return true;
                        }
                    }
                }
                return false;
            }

            bool IsValid(const util::BitPack32 *kac, size_t kac_count) const {
                return IsSingleIdValid(this->GetInterruptId0(), kac, kac_count) && IsSingleIdValid(this->GetInterruptId1(), kac, kac_count);
            }
        );

        DEFINE_CAPABILITY_CLASS(ApplicationType,
            DEFINE_CAPABILITY_FIELD(ApplicationType, IdBits, 3);

            bool IsValid(const util::BitPack32 *kac, size_t kac_count) const {
                for (size_t i = 0; i < kac_count; i++) {
                    if (GetCapabilityId(kac[i]) == Id) {
                        const auto restriction = Decode(kac[i]);

                        return restriction.GetValue() == this->GetValue();
                    }
                }
                return false;
            }

            static constexpr util::BitPack32 Encode(u32 app_type) {
                util::BitPack32 encoded{IdBitsValue};
                encoded.Set<ApplicationType>(app_type);
                return encoded;
            }
        );

        DEFINE_CAPABILITY_CLASS(KernelVersion,
            DEFINE_CAPABILITY_FIELD(MinorVersion, IdBits,        4);
            DEFINE_CAPABILITY_FIELD(MajorVersion, MinorVersion, 13);

            bool IsValid(const util::BitPack32 *kac, size_t kac_count) const {
                for (size_t i = 0; i < kac_count; i++) {
                    if (GetCapabilityId(kac[i]) == Id) {
                        const auto restriction = Decode(kac[i]);

                        return restriction.GetValue() == this->GetValue();
                    }
                }
                return false;
            }
        );

        DEFINE_CAPABILITY_CLASS(HandleTable,
            DEFINE_CAPABILITY_FIELD(Size, IdBits, 10);

            bool IsValid(const util::BitPack32 *kac, size_t kac_count) const {
                for (size_t i = 0; i < kac_count; i++) {
                    if (GetCapabilityId(kac[i]) == Id) {
                        const auto restriction = Decode(kac[i]);

                        return this->GetSize() <= restriction.GetSize();
                    }
                }
                return false;
            }
        );

        DEFINE_CAPABILITY_CLASS(DebugFlags,
            DEFINE_CAPABILITY_FIELD(AllowDebug,     IdBits,         1, bool);
            DEFINE_CAPABILITY_FIELD(ForceDebugProd, AllowDebug,     1, bool);
            DEFINE_CAPABILITY_FIELD(ForceDebug,     ForceDebugProd, 1, bool);

            bool IsValid(const util::BitPack32 *kac, size_t kac_count) const {
                u32 total = 0;
                if (this->GetAllowDebug()) { ++total; }
                if (this->GetForceDebugProd()) { ++total; }
                if (this->GetForceDebug()) { ++total; }
                if (total > 1) {
                    return false;
                }

                for (size_t i = 0; i < kac_count; i++) {
                    if (GetCapabilityId(kac[i]) == Id) {
                        const auto restriction = Decode(kac[i]);

                        return (restriction.GetValue() & this->GetValue()) == this->GetValue();
                    }
                }

                return false;
            }

            static constexpr util::BitPack32 Encode(bool allow_debug, bool force_debug_prod, bool force_debug) {
                util::BitPack32 encoded{IdBitsValue};
                encoded.Set<AllowDebug>(allow_debug);
                encoded.Set<ForceDebugProd>(force_debug_prod);
                encoded.Set<ForceDebug>(force_debug);
                return encoded;
            }
        );

    }

    /* Capabilities API. */
    Result TestCapability(const util::BitPack32 *kacd, size_t kacd_count, const util::BitPack32 *kac, size_t kac_count) {
        for (size_t i = 0; i < kac_count; i++) {
            const auto cap = kac[i];
            const auto id = GetCapabilityId(cap);

#define VALIDATE_CASE(id) \
                case CapabilityId::id: \
                    R_UNLESS(Capability##id::Decode(cap).IsValid(kacd, kacd_count), ldr::ResultInvalidCapability##id()); \
                    break
            switch (id) {
                VALIDATE_CASE(KernelFlags);
                VALIDATE_CASE(SyscallMask);
                VALIDATE_CASE(MapPage);
                VALIDATE_CASE(MapRegion);
                VALIDATE_CASE(InterruptPair);
                VALIDATE_CASE(ApplicationType);
                VALIDATE_CASE(KernelVersion);
                VALIDATE_CASE(HandleTable);
                VALIDATE_CASE(DebugFlags);
                case CapabilityId::MapRange:
                    {
                        /* Map Range needs extra logic because there it involves two sequential caps. */
                        i++;
                        R_UNLESS(i < kac_count, ldr::ResultInvalidCapabilityMapRange());
                        R_UNLESS(CapabilityMapRange::Decode(cap).IsValid(kac[i], kacd, kacd_count), ldr::ResultInvalidCapabilityMapRange());
                    }
                    break;
                default:
                    R_UNLESS(id == CapabilityId::Empty, ldr::ResultUnknownCapability());
                    break;
            }
#undef VALIDATE_CASE
        }

        R_SUCCEED();
    }

    u16 MakeProgramInfoFlag(const util::BitPack32 *kac, size_t count) {
        u16 flags = 0;

        for (size_t i = 0; i < count; ++i) {
            const auto cap = kac[i];

            switch (GetCapabilityId(cap)) {
                case CapabilityId::ApplicationType:
                    {
                        const auto app_type = CapabilityApplicationType::Decode(cap).GetApplicationType() & ProgramInfoFlag_ApplicationTypeMask;
                        if (app_type != ProgramInfoFlag_InvalidType) {
                            flags |= app_type;
                        }
                    }
                    break;
                case CapabilityId::DebugFlags:
                    if (CapabilityDebugFlags::Decode(cap).GetAllowDebug()) {
                        flags |= ProgramInfoFlag_AllowDebug;
                    }
                    break;
                default:
                    break;
            }
        }

        return flags;
    }

    void UpdateProgramInfoFlag(u16 flags, util::BitPack32 *kac, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            const auto cap = kac[i];
            switch (GetCapabilityId(cap)) {
                case CapabilityId::ApplicationType:
                    kac[i] = CapabilityApplicationType::Encode(flags & ProgramInfoFlag_ApplicationTypeMask);
                    break;
                case CapabilityId::DebugFlags:
                    kac[i] = CapabilityDebugFlags::Encode((flags & ProgramInfoFlag_AllowDebug) != 0, CapabilityDebugFlags::Decode(cap).GetForceDebugProd(), CapabilityDebugFlags::Decode(cap).GetForceDebug());
                    break;
                default:
                    break;
            }
        }
    }

    void FixDebugCapabilityForHbl(util::BitPack32 *kac, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            const auto cap = kac[i];
            switch (GetCapabilityId(cap)) {
                case CapabilityId::DebugFlags:
                    /* 19.0.0+ disallows more than one flag set; we are always DebugMode for kernel, so ForceDebug is the most powerful/flexible flag to set. */
                    kac[i] = CapabilityDebugFlags::Encode(false, false, true);
                    break;
                default:
                    break;
            }
        }
    }

    void PreProcessCapability(util::BitPack32 *kac, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            const auto cap = kac[i];
            switch (GetCapabilityId(cap)) {
                /* NOTE: Currently, there is no pre-processing necessary. */
                default:
                    break;
            }
        }
    }

}
