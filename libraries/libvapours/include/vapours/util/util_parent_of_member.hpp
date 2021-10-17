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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util/util_typed_storage.hpp>

namespace ams::util {

    namespace impl {

        #define AMS_UTIL_OFFSET_OF_STANDARD_COMPLIANT 0

        #if AMS_UTIL_OFFSET_OF_STANDARD_COMPLIANT

            template<size_t MaxDepth>
            struct OffsetOfUnionHolder {
                template<typename ParentType, typename MemberType, size_t Offset>
                union UnionImpl {
                    using PaddingMember = char;
                    static constexpr size_t GetOffset() { return Offset; }

                    #pragma pack(push, 1)
                    struct {
                        PaddingMember padding[Offset];
                        MemberType members[(sizeof(ParentType) / sizeof(MemberType)) + 1];
                    } data;
                    #pragma pack(pop)
                    UnionImpl<ParentType, MemberType, Offset + 1> next_union;
                };

                template<typename ParentType, typename MemberType>
                union UnionImpl<ParentType, MemberType, MaxDepth> { /* Empty ... */ };
            };

            template<typename ParentType, typename MemberType>
            struct OffsetOfCalculator {
                using UnionHolder = typename OffsetOfUnionHolder<sizeof(MemberType)>::template UnionImpl<ParentType, MemberType, 0>;
                union Union {
                    char c;
                    UnionHolder first_union;
                    ParentType parent;

                    /* This coerces the active member to be c. */
                    constexpr Union() : c() { /* ... */ }
                    constexpr ~Union() { std::destroy_at(std::addressof(c)); }
                };
                static constexpr Union U = {};

                static constexpr const MemberType *GetNextAddress(const MemberType *start, const MemberType *target) {
                    while (start < target) {
                        start++;
                    }
                    return start;
                }

                static constexpr std::ptrdiff_t GetDifference(const MemberType *start, const MemberType *target) {
                    return (target - start) * sizeof(MemberType);
                }

                template<typename CurUnion>
                static constexpr std::ptrdiff_t OffsetOfImpl(MemberType ParentType::*member, CurUnion &cur_union) {
                    constexpr size_t Offset = CurUnion::GetOffset();
                    const auto target = std::addressof(U.parent.*member);
                    const auto start  = std::addressof(cur_union.data.members[0]);
                    const auto next   = GetNextAddress(start, target);

                    if (next != target) {
                        if constexpr (Offset < sizeof(MemberType) - 1) {
                            return OffsetOfImpl(member, cur_union.next_union);
                        } else {
                            __builtin_unreachable();
                        }
                    }

                    return (next - start) * sizeof(MemberType) + Offset;
                }


                static constexpr std::ptrdiff_t OffsetOf(MemberType ParentType::*member) {
                    return OffsetOfImpl(member, U.first_union);
                }
            };

        #else

            template<typename T>
            union HelperUnion {
                T v;
                char c;

                constexpr HelperUnion() : c() { /* ... */ }
                constexpr ~HelperUnion() { std::destroy_at(std::addressof(c)); }
            };

            template<typename ParentType, typename MemberType>
            struct OffsetOfCalculator {
                static constexpr std::ptrdiff_t OffsetOf(MemberType ParentType::*member) {
                    constexpr HelperUnion<ParentType> Holder = {};
                    const auto *parent = std::addressof(Holder.v);
                    const auto *target = std::addressof(parent->*member);
                    return static_cast<const uint8_t *>(static_cast<const void *>(target)) - static_cast<const uint8_t *>(static_cast<const void *>(parent));
                }
            };

        #endif

        template<typename T>
        struct GetMemberPointerTraits;

        template<typename P, typename M>
        struct GetMemberPointerTraits<M P::*> {
            using Parent = P;
            using Member = M;
        };

        template<auto MemberPtr>
        using GetParentType = typename GetMemberPointerTraits<decltype(MemberPtr)>::Parent;

        template<auto MemberPtr>
        using GetMemberType = typename GetMemberPointerTraits<decltype(MemberPtr)>::Member;

        template<auto MemberPtr, typename RealParentType = GetParentType<MemberPtr>> requires (std::derived_from<RealParentType, GetParentType<MemberPtr>> || std::same_as<RealParentType, GetParentType<MemberPtr>>)
        struct OffsetOf {
            using MemberType = GetMemberType<MemberPtr>;

            static constexpr std::ptrdiff_t Value = OffsetOfCalculator<RealParentType, MemberType>::OffsetOf(MemberPtr);
        };
    }

    template<auto MemberPtr, typename RealParentType = impl::GetParentType<MemberPtr>>
    ALWAYS_INLINE RealParentType &GetParentReference(impl::GetMemberType<MemberPtr> *member) {
        constexpr std::ptrdiff_t Offset = impl::OffsetOf<MemberPtr, RealParentType>::Value;
        return *static_cast<RealParentType *>(static_cast<void *>(static_cast<uint8_t *>(static_cast<void *>(member)) - Offset));
    }

    template<auto MemberPtr, typename RealParentType = impl::GetParentType<MemberPtr>>
    ALWAYS_INLINE RealParentType const &GetParentReference(impl::GetMemberType<MemberPtr> const *member) {
        constexpr std::ptrdiff_t Offset = impl::OffsetOf<MemberPtr, RealParentType>::Value;
        return *static_cast<const RealParentType *>(static_cast<const void *>(static_cast<const uint8_t *>(static_cast<const void *>(member)) - Offset));
    }

    template<auto MemberPtr, typename RealParentType = impl::GetParentType<MemberPtr>>
    ALWAYS_INLINE RealParentType *GetParentPointer(impl::GetMemberType<MemberPtr> *member) {
        return std::addressof(GetParentReference<MemberPtr, RealParentType>(member));
    }

    template<auto MemberPtr, typename RealParentType = impl::GetParentType<MemberPtr>>
    ALWAYS_INLINE RealParentType const *GetParentPointer(impl::GetMemberType<MemberPtr> const *member) {
        return std::addressof(GetParentReference<MemberPtr, RealParentType>(member));
    }

    template<auto MemberPtr, typename RealParentType = impl::GetParentType<MemberPtr>>
    ALWAYS_INLINE RealParentType &GetParentReference(impl::GetMemberType<MemberPtr> &member) {
        return GetParentReference<MemberPtr, RealParentType>(std::addressof(member));
    }

    template<auto MemberPtr, typename RealParentType = impl::GetParentType<MemberPtr>>
    ALWAYS_INLINE RealParentType const &GetParentReference(impl::GetMemberType<MemberPtr> const &member) {
        return GetParentReference<MemberPtr, RealParentType>(std::addressof(member));
    }

    template<auto MemberPtr, typename RealParentType = impl::GetParentType<MemberPtr>>
    ALWAYS_INLINE RealParentType *GetParentPointer(impl::GetMemberType<MemberPtr> &member) {
        return std::addressof(GetParentReference<MemberPtr, RealParentType>(member));
    }

    template<auto MemberPtr, typename RealParentType = impl::GetParentType<MemberPtr>>
    ALWAYS_INLINE RealParentType const *GetParentPointer(impl::GetMemberType<MemberPtr> const &member) {
        return std::addressof(GetParentReference<MemberPtr, RealParentType>(member));
    }


    /* Defines, for use by other code. */

    #define OFFSETOF(parent, member) (::ams::util::impl::OffsetOf<&parent::member, parent>::Value)

    #define GET_PARENT_PTR(parent, member, _arg) (::ams::util::GetParentPointer<&parent::member, parent>(_arg))

    #define GET_PARENT_REF(parent, member, _arg) (::ams::util::GetParentReference<&parent::member, parent>(_arg))

}
