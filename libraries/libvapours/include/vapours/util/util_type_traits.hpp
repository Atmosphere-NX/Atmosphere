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

namespace ams::util {

    template<typename T>
    using is_pod = std::bool_constant<std::is_standard_layout<T>::value && std::is_trivial<T>::value>;

    struct ConstantInitializeTag final {};
    constexpr inline const ConstantInitializeTag ConstantInitialize{};

    namespace impl {

        constexpr int ToIntegerForIsConstexprConstructible(...) { return {}; }

        template<typename T, auto...Lambdas> requires (std::is_constructible<T, decltype(Lambdas())...>::value)
        using ToIntegralConstantForIsConstexprConstructible = std::integral_constant<int, ToIntegerForIsConstexprConstructible(T(Lambdas()...))>;

        template<typename T, auto...Lambdas, int = ToIntegralConstantForIsConstexprConstructible<T, Lambdas...>::value>
        std::true_type IsConstexprConstructibleImpl(int);

        template<typename T, auto...Lambdas>
        std::false_type IsConstexprConstructibleImpl(long);

        template<typename T>
        consteval inline auto ConvertToLambdaForIsConstexprConstructible() { return [] { return T{}; }; }

        template<auto V>
        consteval inline auto ConvertToLambdaForIsConstexprConstructible() { return [] { return V; }; }

        namespace ambiguous_parse {

            struct AmbiguousParseHelperForIsConstexprConstructible {

                constexpr inline AmbiguousParseHelperForIsConstexprConstructible operator-() { return *this; }

                template<typename T>
                constexpr inline operator T() {
                    return T{};
                }
            };

            constexpr inline auto operator -(auto v, AmbiguousParseHelperForIsConstexprConstructible) { return v; }

        }

        #define AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(TYPE_OR_VALUE) [] { ::ams::util::impl::ambiguous_parse::AmbiguousParseHelperForIsConstexprConstructible p; auto v = (TYPE_OR_VALUE)-p; return v; }

    }

    template<typename T, typename...ArgTypes>
    using is_constexpr_constructible = decltype(impl::IsConstexprConstructibleImpl<T, impl::ConvertToLambdaForIsConstexprConstructible<ArgTypes>()...>(0));

    template<typename T, auto...Args>
    using is_constexpr_constructible_by_values = decltype(impl::IsConstexprConstructibleImpl<T, impl::ConvertToLambdaForIsConstexprConstructible<Args>()...>(0));

    #define AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE_1(_1) \
        (decltype(::ams::util::impl::IsConstexprConstructibleImpl<_1>(0))::value)

    #define AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE_2(_1, _2) \
        (decltype(::ams::util::impl::IsConstexprConstructibleImpl<_1, \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_2) \
        >(0))::value)

    #define AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE_3(_1, _2, _3) \
        (decltype(::ams::util::impl::IsConstexprConstructibleImpl<_1, \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_2), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_3) \
        >(0))::value)

    #define AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE_4(_1, _2, _3, _4) \
        (decltype(::ams::util::impl::IsConstexprConstructibleImpl<_1, \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_2), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_3), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_4) \
        >(0))::value)

    #define AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE_5(_1, _2, _3, _4, _5) \
        (decltype(::ams::util::impl::IsConstexprConstructibleImpl<_1, \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_2), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_3), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_4), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_5) \
        >(0))::value)

    #define AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE_6(_1, _2, _3, _4, _5, _6) \
        (decltype(::ams::util::impl::IsConstexprConstructibleImpl<_1, \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_2), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_3), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_4), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_5), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_6) \
        >(0))::value)

    #define AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE_7(_1, _2, _3, _4, _5, _6, _7) \
        (decltype(::ams::util::impl::IsConstexprConstructibleImpl<_1, \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_2), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_3), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_4), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_5), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_6), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_7) \
        >(0))::value)

    #define AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE_8(_1, _2, _3, _4, _5, _6, _7, _8) \
        (decltype(::ams::util::impl::IsConstexprConstructibleImpl<_1, \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_2), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_3), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_4), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_5), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_6), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_7), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_8) \
        >(0))::value)

    #define AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE_9(_1, _2, _3, _4, _5, _6, _7, _8, _9) \
        (decltype(::ams::util::impl::IsConstexprConstructibleImpl<_1, \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_2), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_3), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_4), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_5), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_6), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_7), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_8), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_9) \
        >(0))::value)

    #define AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE_10(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10) \
        (decltype(::ams::util::impl::IsConstexprConstructibleImpl<_1, \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_2), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_3), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_4), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_5), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_6), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_7), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_8), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_9), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_10) \
        >(0))::value)

    #define AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE_11(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11) \
        (decltype(::ams::util::impl::IsConstexprConstructibleImpl<_1, \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_2), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_3), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_4), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_5), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_6), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_7), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_8), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_9), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_10), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_11) \
        >(0))::value)

    #define AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE_12(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12) \
        (decltype(::ams::util::impl::IsConstexprConstructibleImpl<_1, \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_2), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_3), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_4), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_5), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_6), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_7), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_8), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_9), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_10), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_11), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_12) \
        >(0))::value)

    #define AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE_13(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13) \
        (decltype(::ams::util::impl::IsConstexprConstructibleImpl<_1, \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_2), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_3), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_4), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_5), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_6), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_7), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_8), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_9), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_10), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_11), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_12), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_13) \
        >(0))::value)

    #define AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE_14(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14) \
        (decltype(::ams::util::impl::IsConstexprConstructibleImpl<_1, \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_2), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_3), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_4), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_5), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_6), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_7), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_8), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_9), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_10), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_11), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_12), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_13), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_14) \
        >(0))::value)

    #define AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE_15(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15) \
        (decltype(::ams::util::impl::IsConstexprConstructibleImpl<_1, \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_2), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_3), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_4), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_5), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_6), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_7), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_8), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_9), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_10), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_11), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_12), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_13), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_14), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_15) \
        >(0))::value)

    #define AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE_16(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16) \
        (decltype(::ams::util::impl::IsConstexprConstructibleImpl<_1, \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_2), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_3), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_4), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_5), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_6), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_7), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_8), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_9), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_10), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_11), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_12), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_13), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_14), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_15), \
            AMS_UTIL_IMPL_CONVERT_TV_TO_LAMBDA(_16) \
        >(0))::value)

    #define AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE(...) AMS_VMACRO(AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE, __VA_ARGS__)

    #if 0
    namespace test {

        struct S {
            private:
                int m_v;
            public:
                S() { }

                constexpr S(int v) : m_v() { }
                constexpr S(int v, double z) : m_v(v) { }
        };

        consteval inline int test_constexpr_int() { return 0; }
        inline int test_not_constexpr_int() { return 0; }

        static_assert(!AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE(S));
        static_assert(AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE(S, int));
        static_assert(AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE(S, 0));
        static_assert(AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE(S, test_constexpr_int()));
        static_assert(!AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE(S, test_not_constexpr_int()));

        static_assert(AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE(S, int, double));
        static_assert(AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE(S, int, 0.0));
        static_assert(AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE(S, 0, double));
        static_assert(AMS_UTIL_IS_CONSTEXPR_CONSTRUCTIBLE(S, 0, 0.0));

    }
    #endif

}
