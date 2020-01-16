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
#include "../defines.hpp"

namespace ams::util {

    namespace impl {

        template<size_t Alignment>
        struct OffsetOfUnionHolder {
            template<typename ParentType, typename MemberType, size_t Offset>
            union UnionImpl {
                static constexpr size_t GetOffset() { return Offset; }

                struct {
                    char padding[Offset];
                    MemberType members[(sizeof(ParentType) / sizeof(MemberType)) + 1];
                } data;
                UnionImpl<ParentType, MemberType, Offset + 1> next_union;
            };

            template<typename ParentType, typename MemberType>
            union UnionImpl<ParentType, MemberType, 0> {
                static constexpr size_t GetOffset() { return 0; }

                struct {
                    MemberType members[(sizeof(ParentType) / sizeof(MemberType)) + 1];
                } data;
                UnionImpl<ParentType, MemberType, 1> next_union;
            };

            template<typename ParentType, typename MemberType>
            union UnionImpl<ParentType, MemberType, Alignment> { /* Empty */ };
        };

        template<typename ParentType, typename MemberType>
        struct OffsetOfCalculator {
            using UnionHolder = typename OffsetOfUnionHolder<alignof(MemberType)>::template UnionImpl<ParentType, MemberType, 0>;
            union Union {
                char c;
                UnionHolder first_union;
                ParentType parent;

                /* This coerces the active member to be c. */
                constexpr Union() : c() { /* ... */ }
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

                if (next < target) {
                    if constexpr (Offset + 1 < alignof(MemberType)) {
                        return OffsetOfImpl(member, cur_union.next_union);
                    } else {
                        static_assert(Offset + 1 <= alignof(MemberType));
                    }
                }

                return (next - start) * sizeof(MemberType) + Offset;
            }


            static constexpr std::ptrdiff_t OffsetOf(MemberType ParentType::*member) {
                return OffsetOfImpl(member, U.first_union);
            }
        };

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

        template<auto MemberPtr, typename RealParentType = GetParentType<MemberPtr>>
        constexpr inline std::ptrdiff_t OffsetOf = [] {
            using DeducedParentType = GetParentType<MemberPtr>;
            using MemberType        = GetMemberType<MemberPtr>;
            static_assert(std::is_base_of<DeducedParentType, RealParentType>::value || std::is_same<RealParentType, DeducedParentType>::value);

            return OffsetOfCalculator<RealParentType, MemberType>::OffsetOf(MemberPtr);
        }();

    }

    template<auto MemberPtr, typename RealParentType = impl::GetParentType<MemberPtr>>
    constexpr ALWAYS_INLINE RealParentType &GetParentReference(impl::GetMemberType<MemberPtr> *member) {
        constexpr std::ptrdiff_t Offset = impl::OffsetOf<MemberPtr, RealParentType>;
        uint8_t *addr = static_cast<uint8_t *>(static_cast<void *>(member));
        addr -= Offset;
        return *static_cast<RealParentType *>(static_cast<void *>(addr));
    }

    template<auto MemberPtr, typename RealParentType = impl::GetParentType<MemberPtr>>
    constexpr ALWAYS_INLINE RealParentType const &GetParentReference(impl::GetMemberType<MemberPtr> const *member) {
        constexpr std::ptrdiff_t Offset = impl::OffsetOf<MemberPtr, RealParentType>;
        const uint8_t *addr = static_cast<const uint8_t *>(static_cast<const void *>(member));
        addr -= Offset;
        return *static_cast<const RealParentType *>(static_cast<const void *>(addr));
    }

    template<auto MemberPtr, typename RealParentType = impl::GetParentType<MemberPtr>>
    constexpr ALWAYS_INLINE RealParentType *GetParentPointer(impl::GetMemberType<MemberPtr> *member) {
        return std::addressof(GetParentReference<MemberPtr, RealParentType>(member));
    }

    template<auto MemberPtr, typename RealParentType = impl::GetParentType<MemberPtr>>
    constexpr ALWAYS_INLINE RealParentType const *GetParentPointer(impl::GetMemberType<MemberPtr> const *member) {
        return std::addressof(GetParentReference<MemberPtr, RealParentType>(member));
    }

    namespace test {

        struct Struct1 {
            uint32_t a;
        };

        struct Struct2 {
            uint32_t b;
        };

        struct Struct3 : public Struct1, Struct2 {
            uint32_t c;
        };

        static_assert(impl::OffsetOf<&Struct1::a> == 0);
        static_assert(impl::OffsetOf<&Struct2::b> == 0);
        static_assert(impl::OffsetOf<&Struct3::a> == 0);
        static_assert(impl::OffsetOf<&Struct3::b> == 0);


        static_assert(impl::OffsetOf<&Struct3::a, Struct3> == 0 || impl::OffsetOf<&Struct3::b, Struct3> == 0);
        static_assert(impl::OffsetOf<&Struct3::a, Struct3> == sizeof(Struct2) || impl::OffsetOf<&Struct3::b, Struct3> == sizeof(Struct1));
        static_assert(impl::OffsetOf<&Struct3::c> == sizeof(Struct1) + sizeof(Struct2));

        constexpr Struct3 TestStruct3 = {};

        static_assert(std::addressof(TestStruct3) == GetParentPointer<&Struct3::a, Struct3>(std::addressof(TestStruct3.a)));
        static_assert(std::addressof(TestStruct3) == GetParentPointer<&Struct3::b, Struct3>(std::addressof(TestStruct3.b)));
        static_assert(std::addressof(TestStruct3) == GetParentPointer<&Struct3::c, Struct3>(std::addressof(TestStruct3.c)));

        struct CharArray {
            char c0;
            char c1;
            char c2;
            char c3;
            char c4;
            char c5;
            char c6;
            char c7;
        };

        static_assert(impl::OffsetOf<&CharArray::c0> == 0);
        static_assert(impl::OffsetOf<&CharArray::c1> == 1);
        static_assert(impl::OffsetOf<&CharArray::c2> == 2);
        static_assert(impl::OffsetOf<&CharArray::c3> == 3);
        static_assert(impl::OffsetOf<&CharArray::c4> == 4);
        static_assert(impl::OffsetOf<&CharArray::c5> == 5);
        static_assert(impl::OffsetOf<&CharArray::c6> == 6);
        static_assert(impl::OffsetOf<&CharArray::c7> == 7);

        constexpr CharArray TestCharArray = {};

        static_assert(std::addressof(TestCharArray) == GetParentPointer<&CharArray::c0>(std::addressof(TestCharArray.c0)));
        static_assert(std::addressof(TestCharArray) == GetParentPointer<&CharArray::c1>(std::addressof(TestCharArray.c1)));
        static_assert(std::addressof(TestCharArray) == GetParentPointer<&CharArray::c2>(std::addressof(TestCharArray.c2)));
        static_assert(std::addressof(TestCharArray) == GetParentPointer<&CharArray::c3>(std::addressof(TestCharArray.c3)));
        static_assert(std::addressof(TestCharArray) == GetParentPointer<&CharArray::c4>(std::addressof(TestCharArray.c4)));
        static_assert(std::addressof(TestCharArray) == GetParentPointer<&CharArray::c5>(std::addressof(TestCharArray.c5)));
        static_assert(std::addressof(TestCharArray) == GetParentPointer<&CharArray::c6>(std::addressof(TestCharArray.c6)));
        static_assert(std::addressof(TestCharArray) == GetParentPointer<&CharArray::c7>(std::addressof(TestCharArray.c7)));

    }

}

/* Defines, for use by other code. */

#define OFFSETOF(parent, member) (::ams::util::OffsetOf<&parent::member, parent>)

#define AMS_GET_PARENT_TYPE_IMPL(parent, argt) typename std::conditional<std::is_pointer<argt>::value,                                                          \
                                                                    typename std::conditional<std::is_const<typename std::remove_pointer<argt>::type>::value,   \
                                                                                              const parent,                                                     \
                                                                                              parent                                                            \
                                                                                             >::type,                                                           \
                                                                    typename std::conditional<std::is_const<typename std::remove_reference<argt>::type>::value, \
                                                                                              const parent,                                                     \
                                                                                              parent                                                            \
                                                                                             >::type                                                            \
                                                                    >::type

#define GET_PARENT_PTR(parent, member, _arg) ([]<typename GetParentPtrArgType>(GetParentPtrArgType &arg) ALWAYS_INLINE_LAMBDA -> AMS_GET_PARENT_TYPE_IMPL(parent, GetParentPtrArgType) * {     \
    using ParentPtrType = typename std::conditional<std::is_const<GetParentPtrArgType>::value, const parent, parent>::type *;                                                                  \
    if constexpr (std::is_pointer<GetParentPtrArgType>::value) {                                                                                                                               \
        static_assert(std::is_same<typename std::remove_cv<decltype(parent::member)>::type, typename std::remove_cv<typename std::remove_pointer<GetParentPtrArgType>::type>::type>::value);   \
        return ::ams::util::GetParentPointer<&parent::member, parent>(arg);                                                                                                                    \
    } else {                                                                                                                                                                                   \
        static_assert(std::is_same<typename std::remove_cv<decltype(parent::member)>::type, typename std::remove_cv<typename std::remove_reference<GetParentPtrArgType>::type>::type>::value); \
        return ::ams::util::GetParentPointer<&parent::member, parent>(std::addressof(arg));                                                                                                    \
    }                                                                                                                                                                                          \
}(_arg))

#define GET_PARENT_REF(parent, member, _arg) ([]<typename GetParentPtrArgType>(GetParentPtrArgType &arg) ALWAYS_INLINE_LAMBDA -> AMS_GET_PARENT_TYPE_IMPL(parent, GetParentPtrArgType) & {     \
    if constexpr (std::is_pointer<GetParentPtrArgType>::value) {                                                                                                                               \
        static_assert(std::is_same<typename std::remove_cv<decltype(parent::member)>::type, typename std::remove_cv<typename std::remove_pointer<GetParentPtrArgType>::type>::type>::value);   \
        return ::ams::util::GetParentReference<&parent::member, parent>(arg);                                                                                                                  \
    } else {                                                                                                                                                                                   \
        static_assert(std::is_same<typename std::remove_cv<decltype(parent::member)>::type, typename std::remove_cv<typename std::remove_reference<GetParentPtrArgType>::type>::type>::value); \
        return ::ams::util::GetParentReference<&parent::member, parent>(std::addressof(arg));                                                                                                  \
    }                                                                                                                                                                                          \
}(_arg))
