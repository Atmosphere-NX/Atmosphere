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
#include "ldr_capabilities.hpp"

namespace ams::ldr::caps {

    namespace {

        /* Types. */
        enum class CapabilityId {
            KernelFlags     = 3,
            SyscallMask     = 4,
            MapRange        = 6,
            MapPage         = 7,
            InterruptPair   = 11,
            ApplicationType = 13,
            KernelVersion   = 14,
            HandleTable     = 15,
            DebugFlags      = 16,

            Empty = 32,
        };

        constexpr CapabilityId GetCapabilityId(u32 cap) {
            return static_cast<CapabilityId>(__builtin_ctz(~cap));
        }

        template<CapabilityId Id>
        class Capability {
            public:
                static constexpr u32 ValueShift = static_cast<u32>(Id) + 1;
                static constexpr u32 IdMask = (1u << (ValueShift - 1)) - 1;
            private:
                u32 value;
            public:
                Capability(u32 v) : value(v) { /* ... */ }

                CapabilityId GetId() const {
                    return Id;
                }

                u32 GetValue() const {
                    return this->value >> ValueShift;
                }
        };

#define CAPABILITY_CLASS_NAME(id) Capability##id
#define CAPABILITY_BASE_CLASS(id) Capability<CapabilityId::id>

#define DEFINE_CAPABILITY_CLASS(id, member_functions) \
        class CAPABILITY_CLASS_NAME(id) : public CAPABILITY_BASE_CLASS(id) { \
            public: \
                CAPABILITY_CLASS_NAME(id)(u32 v) : CAPABILITY_BASE_CLASS(id)(v) { /* ... */ } \
\
                static CAPABILITY_CLASS_NAME(id) Decode(u32 v) { return CAPABILITY_CLASS_NAME(id)(v); } \
\
                member_functions \
        }

        /* Class definitions. */
        DEFINE_CAPABILITY_CLASS(KernelFlags,
            u32 GetMaximumThreadPriority() const {
                return (this->GetValue() >> 0) & 0x3F;
            }

            u32 GetMinimumThreadPriority() const {
                return (this->GetValue() >> 6) & 0x3F;
            }

            u32 GetMinimumCoreId() const {
                return (this->GetValue() >> 12) & 0xFF;
            }

            u32 GetMaximumCoreId() const {
                return (this->GetValue() >> 20) & 0xFF;
            }

            bool IsValid(const u32 *kac, size_t kac_count) const {
                for (size_t i = 0; i < kac_count; i++) {
                    if (GetCapabilityId(kac[i]) == this->GetId()) {
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
            u32 GetMask() const {
                return (this->GetValue() >> 0) & 0xFFFFFF;
            }

            u32 GetIndex() const {
                return (this->GetValue() >> 24) & 0x7;
            }

            bool IsValid(const u32 *kac, size_t kac_count) const {
                for (size_t i = 0; i < kac_count; i++) {
                    if (GetCapabilityId(kac[i]) == this->GetId()) {
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
            static constexpr size_t SizeMax = 0x100000;

            u32 GetAddressSize() const {
                return (this->GetValue() >> 0) & 0xFFFFFF;
            }

            u32 GetFlag() const {
                return (this->GetValue() >> 24) & 0x1;
            }

            bool IsValid(const u32 next_cap, const u32 *kac, size_t kac_count) const {
                if (GetCapabilityId(next_cap) != this->GetId()) {
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
                    if (GetCapabilityId(kac[i]) == this->GetId()) {
                        const auto restriction = Decode(kac[i++]);
                        if (i >= kac_count || GetCapabilityId(kac[i]) != this->GetId()) {
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
            u32 GetAddress() const {
                return (this->GetValue() >> 0) & 0xFFFFFF;
            }

            bool IsValid(const u32 *kac, size_t kac_count) const {
                for (size_t i = 0; i < kac_count; i++) {
                    if (GetCapabilityId(kac[i]) == this->GetId()) {
                        const auto restriction = Decode(kac[i]);

                        if (this->GetValue() == restriction.GetValue()) {
                            return true;
                        }
                    }
                }
                return false;
            }
        );

        DEFINE_CAPABILITY_CLASS(InterruptPair,
            static constexpr u32 EmptyInterruptId = 0x3FF;

            u32 GetInterruptId0() const {
                return (this->GetValue() >> 0) & 0x3FF;
            }

            u32 GetInterruptId1() const {
                return (this->GetValue() >> 10) & 0x3FF;
            }

            bool IsSingleIdValid(const u32 id, const u32 *kac, size_t kac_count) const {
                for (size_t i = 0; i < kac_count; i++) {
                    if (GetCapabilityId(kac[i]) == this->GetId()) {
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

            bool IsValid(const u32 *kac, size_t kac_count) const {
                return IsSingleIdValid(this->GetInterruptId0(), kac, kac_count) && IsSingleIdValid(this->GetInterruptId1(), kac, kac_count);
            }
        );

        DEFINE_CAPABILITY_CLASS(ApplicationType,
            u32 GetApplicationType() const {
                return (this->GetValue() >> 0) & 0x3;
            }

            bool IsValid(const u32 *kac, size_t kac_count) const {
                for (size_t i = 0; i < kac_count; i++) {
                    if (GetCapabilityId(kac[i]) == this->GetId()) {
                        const auto restriction = Decode(kac[i]);

                        return restriction.GetValue() == this->GetValue();
                    }
                }
                return false;
            }

            static constexpr u32 Encode(u32 app_type) {
                return ((app_type & 3) << ValueShift) | IdMask;
            }
        );

        DEFINE_CAPABILITY_CLASS(KernelVersion,
            u32 GetMinorVersion() const {
                return (this->GetValue() >> 0) & 0xF;
            }

            u32 GetMajorVersion() const {
                /* TODO: Are upper bits unused? */
                return (this->GetValue() >> 4) & 0x1FFF;
            }

            bool IsValid(const u32 *kac, size_t kac_count) const {
                for (size_t i = 0; i < kac_count; i++) {
                    if (GetCapabilityId(kac[i]) == this->GetId()) {
                        const auto restriction = Decode(kac[i]);

                        return restriction.GetValue() == this->GetValue();
                    }
                }
                return false;
            }
        );

        DEFINE_CAPABILITY_CLASS(HandleTable,
            u32 GetSize() const {
                return (this->GetValue() >> 0) & 0x3FF;
            }

            bool IsValid(const u32 *kac, size_t kac_count) const {
                for (size_t i = 0; i < kac_count; i++) {
                    if (GetCapabilityId(kac[i]) == this->GetId()) {
                        const auto restriction = Decode(kac[i]);

                        return this->GetSize() <= restriction.GetSize();
                    }
                }
                return false;
            }
        );

        DEFINE_CAPABILITY_CLASS(DebugFlags,
            bool GetAllowDebug() const {
                return (this->GetValue() >> 0) & 1;
            }

            bool GetForceDebug() const {
                return (this->GetValue() >> 1) & 1;
            }

            bool IsValid(const u32 *kac, size_t kac_count) const {
                for (size_t i = 0; i < kac_count; i++) {
                    if (GetCapabilityId(kac[i]) == this->GetId()) {
                        const auto restriction = Decode(kac[i]);

                        return (restriction.GetValue() & this->GetValue()) == this->GetValue();
                    }
                }
                return false;
            }

            static constexpr u32 Encode(bool allow_debug, bool force_debug) {
                const u32 desc = (static_cast<u32>(force_debug) << 1) | (static_cast<u32>(allow_debug) << 0);
                return (desc << ValueShift) | IdMask;
            }
        );

    }

    /* Capabilities API. */
    Result ValidateCapabilities(const void *acid_kac, size_t acid_kac_size, const void *aci_kac, size_t aci_kac_size) {
        const u32 *acid_caps = reinterpret_cast<const u32 *>(acid_kac);
        const u32 *aci_caps = reinterpret_cast<const u32 *>(aci_kac);
        const size_t num_acid_caps = acid_kac_size / sizeof(*acid_caps);
        const size_t num_aci_caps = aci_kac_size / sizeof(*aci_caps);

        for (size_t i = 0; i < num_aci_caps; i++) {
            const u32 cur_cap = aci_caps[i];
            const auto id = GetCapabilityId(cur_cap);

#define VALIDATE_CASE(id) \
                case CapabilityId::id: \
                    R_UNLESS(Capability##id::Decode(cur_cap).IsValid(acid_caps, num_acid_caps), ldr::ResultInvalidCapability##id()); \
                    break
            switch (id) {
                VALIDATE_CASE(KernelFlags);
                VALIDATE_CASE(SyscallMask);
                VALIDATE_CASE(MapPage);
                VALIDATE_CASE(InterruptPair);
                VALIDATE_CASE(ApplicationType);
                VALIDATE_CASE(KernelVersion);
                VALIDATE_CASE(HandleTable);
                VALIDATE_CASE(DebugFlags);
                case CapabilityId::MapRange:
                    {
                        /* Map Range needs extra logic because there it involves two sequential caps. */
                        i++;
                        R_UNLESS(i < num_aci_caps, ldr::ResultInvalidCapabilityMapRange());
                        R_UNLESS(CapabilityMapRange::Decode(cur_cap).IsValid(aci_caps[i], acid_caps, num_acid_caps), ldr::ResultInvalidCapabilityMapRange());
                    }
                    break;
                default:
                    R_UNLESS(id == CapabilityId::Empty, ldr::ResultUnknownCapability());
                    break;
            }
#undef VALIDATE_CASE
        }

        return ResultSuccess();
    }

    u16 GetProgramInfoFlags(const void *kac, size_t kac_size) {
        const u32 *caps = reinterpret_cast<const u32 *>(kac);
        const size_t num_caps = kac_size / sizeof(*caps);
        u16 flags = 0;

        for (size_t i = 0; i < num_caps; i++) {
            const u32 cur_cap = caps[i];

            switch (GetCapabilityId(cur_cap)) {
                case CapabilityId::ApplicationType:
                    {
                        const auto app_type = CapabilityApplicationType::Decode(cur_cap).GetApplicationType() & ProgramInfoFlag_ApplicationTypeMask;
                        if (app_type != ProgramInfoFlag_InvalidType) {
                            flags |= app_type;
                        }
                    }
                    break;
                case CapabilityId::DebugFlags:
                    if (CapabilityDebugFlags::Decode(cur_cap).GetAllowDebug()) {
                        flags |= ProgramInfoFlag_AllowDebug;
                    }
                    break;
                default:
                    break;
            }
        }

        return flags;
    }

    void SetProgramInfoFlags(u16 flags, void *kac, size_t kac_size) {
        u32 *caps = reinterpret_cast<u32 *>(kac);
        const size_t num_caps = kac_size / sizeof(*caps);

        for (size_t i = 0; i < num_caps; i++) {
            const u32 cur_cap = caps[i];
            switch (GetCapabilityId(cur_cap)) {
                case CapabilityId::ApplicationType:
                    caps[i] = CapabilityApplicationType::Encode(flags & ProgramInfoFlag_ApplicationTypeMask);
                    break;
                case CapabilityId::DebugFlags:
                    caps[i] = CapabilityDebugFlags::Encode((flags & ProgramInfoFlag_AllowDebug) != 0, CapabilityDebugFlags::Decode(cur_cap).GetForceDebug());
                    break;
                default:
                    break;
            }
        }
    }

}
