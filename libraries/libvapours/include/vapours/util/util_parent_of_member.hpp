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

        #define AMS_UTIL_OFFSET_OF_STANDARD_COMPLIANT 1

        #if AMS_UTIL_OFFSET_OF_STANDARD_COMPLIANT

            template<std::ptrdiff_t Offset, typename P, typename M, auto Ptr>
            consteval std::strong_ordering TestOffsetForOffsetOfImpl() {
                #pragma pack(push, 1)
                const union Union {
                    char c;
                    struct {
                        char padding[Offset];
                        M members[1 + (sizeof(P) / std::max<size_t>(sizeof(M), 1))];
                    };
                    P p;

                    constexpr Union() : c() { /* ... */ }
                    constexpr ~Union() { /* ... */ }
                } U;
                #pragma pack(pop)

                const M *target = std::addressof(U.p.*Ptr);
                const M *guess  = std::addressof(U.members[0]);

                /* NOTE: target == guess is definitely legal, target < guess is probably legal, definitely legal if Offset <= true offsetof. */
                /* <=> may or may not be legal, but it definitely seems to work. Evaluate again, if it breaks. */
                return guess <=> target;

                //if (guess == target) {
                //    return std::strong_ordering::equal;
                //} else if (guess < target) {
                //    return std::strong_ordering::less;
                //} else {
                //    return std::strong_ordering::greater;
                //}
            }

            template<std::ptrdiff_t Low, std::ptrdiff_t High, typename P, typename M, auto Ptr>
            consteval std::ptrdiff_t OffsetOfImpl() {
                static_assert(Low <= High);

                constexpr std::ptrdiff_t Guess = (Low + High) / 2;
                constexpr auto Order = TestOffsetForOffsetOfImpl<Guess, P, M, Ptr>();

                if constexpr (Order == std::strong_ordering::equal) {
                    return Guess;
                } else if constexpr (Order == std::strong_ordering::less) {
                    return OffsetOfImpl<Guess + 1, High, P, M, Ptr>();
                } else {
                    static_assert(Order == std::strong_ordering::greater);
                    return OffsetOfImpl<Low, Guess - 1, P, M, Ptr>();
                }
            }

            template<typename P, typename M, auto Ptr>
            struct OffsetOfCalculator {
                static constexpr const std::ptrdiff_t Value = OffsetOfImpl<0, sizeof(P), P, M, Ptr>();
            };

        #else

            template<typename ParentType, typename MemberType, auto Ptr>
            struct OffsetOfCalculator {
                private:
                    static consteval std::ptrdiff_t Calculate() {
                        const union Union {
                            ParentType p;
                            char c;

                            constexpr Union() : c() { /* ... */ }
                            constexpr ~Union() { /* ... */ }
                        } U;

                        const auto *parent = std::addressof(U.p);
                        const auto *target = std::addressof(parent->*Ptr);

                        return static_cast<const uint8_t *>(static_cast<const void *>(target)) - static_cast<const uint8_t *>(static_cast<const void *>(parent));
                    }
                public:
                    static constexpr const std::ptrdiff_t Value = Calculate();
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
        struct OffsetOf : public std::integral_constant<std::ptrdiff_t, OffsetOfCalculator<RealParentType, GetMemberType<MemberPtr>, MemberPtr>::Value> {};

    }

    template<auto MemberPtr, typename RealParentType = impl::GetParentType<MemberPtr>>
    ALWAYS_INLINE RealParentType &GetParentReference(impl::GetMemberType<MemberPtr> *member) {
        constexpr std::ptrdiff_t Offset = impl::OffsetOf<MemberPtr, RealParentType>::value;
        return *static_cast<RealParentType *>(static_cast<void *>(static_cast<uint8_t *>(static_cast<void *>(member)) - Offset));
    }

    template<auto MemberPtr, typename RealParentType = impl::GetParentType<MemberPtr>>
    ALWAYS_INLINE RealParentType const &GetParentReference(impl::GetMemberType<MemberPtr> const *member) {
        constexpr std::ptrdiff_t Offset = impl::OffsetOf<MemberPtr, RealParentType>::value;
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
    #define AMS_OFFSETOF(parent, member) (__builtin_offsetof(parent, member))
    #define AMS_GET_PARENT_PTR(parent, member, _arg) (::ams::util::GetParentPointer<&parent::member, parent>(_arg))
    #define AMS_GET_PARENT_REF(parent, member, _arg) (::ams::util::GetParentReference<&parent::member, parent>(_arg))

}
