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

namespace ams {

    namespace result::impl {

        class ResultTraits {
            public:
                using BaseType = u32;
                static_assert(std::is_same<BaseType, ::Result>::value, "std::is_same<BaseType, ::Result>::value");
                static constexpr BaseType SuccessValue = BaseType();
                static constexpr BaseType ModuleBits = 9;
                static constexpr BaseType DescriptionBits = 13;
                static constexpr BaseType ReservedBits = 10;
                static_assert(ModuleBits + DescriptionBits + ReservedBits == sizeof(BaseType) * CHAR_BIT, "ModuleBits + DescriptionBits + ReservedBits == sizeof(BaseType) * CHAR_BIT");
            public:
                NX_CONSTEXPR BaseType MakeValue(BaseType module, BaseType description) {
                    return (module) | (description << ModuleBits);
                }

                template<BaseType module, BaseType description>
                struct MakeStaticValue : public std::integral_constant<BaseType, MakeValue(module, description)> {
                    static_assert(module < (1 << ModuleBits), "Invalid Module");
                    static_assert(description < (1 << DescriptionBits), "Invalid Description");
                };

                NX_CONSTEXPR BaseType GetModuleFromValue(BaseType value) {
                    return value & ~(~BaseType() << ModuleBits);
                }

                NX_CONSTEXPR BaseType GetDescriptionFromValue(BaseType value) {
                    return ((value >> ModuleBits) & ~(~BaseType() << DescriptionBits));
                }
        };

        /* Use CRTP for Results. */
        template<typename Self>
        class ResultBase {
            public:
                using BaseType = typename ResultTraits::BaseType;
                static constexpr BaseType SuccessValue = ResultTraits::SuccessValue;
            public:
                constexpr inline BaseType GetModule() const { return ResultTraits::GetModuleFromValue(static_cast<const Self *>(this)->GetValue()); }
                constexpr inline BaseType GetDescription() const { return ResultTraits::GetDescriptionFromValue(static_cast<const Self *>(this)->GetValue()); }
        };

        class ResultConstructor;

    }

    class ResultSuccess;

    class Result final : public result::impl::ResultBase<Result> {
        friend class ResultConstructor;
        public:
            using Base = typename result::impl::ResultBase<Result>;
        private:
            typename Base::BaseType value;
        private:
            /* TODO: Maybe one-day, the result constructor. */
        public:
            Result() { /* ... */ }

            /* TODO: It sure would be nice to make this private. */
            constexpr Result(typename Base::BaseType v) : value(v) { static_assert(std::is_same<typename Base::BaseType, ::Result>::value); }

            constexpr inline operator ResultSuccess() const;
            NX_CONSTEXPR bool CanAccept(Result result) { return true; }

            constexpr inline bool IsSuccess() const { return this->GetValue() == Base::SuccessValue; }
            constexpr inline bool IsFailure() const { return !this->IsSuccess(); }
            constexpr inline typename Base::BaseType GetModule() const { return Base::GetModule(); }
            constexpr inline typename Base::BaseType GetDescription() const { return Base::GetDescription(); }

            constexpr inline typename Base::BaseType GetValue() const { return this->value; }
    };
    static_assert(sizeof(Result) == sizeof(Result::Base::BaseType), "sizeof(Result) == sizeof(Result::Base::BaseType)");
    static_assert(std::is_trivially_destructible<Result>::value, "std::is_trivially_destructible<Result>::value");

    namespace result::impl {

        class ResultConstructor {
            public:
                static constexpr inline Result MakeResult(ResultTraits::BaseType value) {
                    return Result(value);
                }
        };

        constexpr inline Result MakeResult(ResultTraits::BaseType value) {
            return ResultConstructor::MakeResult(value);
        }

    }

    class ResultSuccess final : public result::impl::ResultBase<ResultSuccess> {
        public:
            using Base = typename result::impl::ResultBase<ResultSuccess>;
        public:
            constexpr operator Result() const { return result::impl::MakeResult(Base::SuccessValue); }
            NX_CONSTEXPR bool CanAccept(Result result) { return result.IsSuccess(); }

            constexpr inline bool IsSuccess() const { return true; }
            constexpr inline bool IsFailure() const { return !this->IsSuccess(); }
            constexpr inline typename Base::BaseType GetModule() const { return Base::GetModule(); }
            constexpr inline typename Base::BaseType GetDescription() const { return Base::GetDescription(); }

            constexpr inline typename Base::BaseType GetValue() const { return Base::SuccessValue; }
    };

    namespace result::impl {

        NORETURN void OnResultAssertion(Result result);

    }

    constexpr inline Result::operator ResultSuccess() const {
        if (!ResultSuccess::CanAccept(*this)) {
            result::impl::OnResultAssertion(*this);
        }
        return ResultSuccess();
    }

    namespace result::impl {

        template<ResultTraits::BaseType _Module, ResultTraits::BaseType _Description>
        class ResultErrorBase : public ResultBase<ResultErrorBase<_Module, _Description>> {
            public:
                using Base = typename result::impl::ResultBase<ResultErrorBase<_Module, _Description>>;
                static constexpr typename Base::BaseType Module = _Module;
                static constexpr typename Base::BaseType Description = _Description;
                static constexpr typename Base::BaseType Value = ResultTraits::MakeStaticValue<Module, Description>::value;
                static_assert(Value != Base::SuccessValue, "Value != Base::SuccessValue");
            public:
                constexpr operator Result() const { return MakeResult(Value); }
                constexpr operator ResultSuccess() const { OnResultAssertion(Value); }

                constexpr inline bool IsSuccess() const { return false; }
                constexpr inline bool IsFailure() const { return !this->IsSuccess(); }

                constexpr inline typename Base::BaseType GetValue() const { return Value; }
        };

        template<ResultTraits::BaseType _Module, ResultTraits::BaseType DescStart, ResultTraits::BaseType DescEnd>
        class ResultErrorRangeBase {
            public:
                static constexpr ResultTraits::BaseType Module = _Module;
                static constexpr ResultTraits::BaseType DescriptionStart = DescStart;
                static constexpr ResultTraits::BaseType DescriptionEnd = DescEnd;
                static_assert(DescriptionStart <= DescriptionEnd, "DescriptionStart <= DescriptionEnd");
                static constexpr typename ResultTraits::BaseType StartValue = ResultTraits::MakeStaticValue<Module, DescriptionStart>::value;
                static constexpr typename ResultTraits::BaseType EndValue = ResultTraits::MakeStaticValue<Module, DescriptionEnd>::value;
            public:
                NX_CONSTEXPR bool Includes(Result result) {
                    return StartValue <= result.GetValue() && result.GetValue() <= EndValue;
                }
        };

    }

}

/* Macros for defining new results. */
#define R_DEFINE_NAMESPACE_RESULT_MODULE(value) namespace impl::result { static constexpr inline ::ams::result::impl::ResultTraits::BaseType ResultModuleId = value; }
#define R_CURRENT_NAMESPACE_RESULT_MODULE impl::result::ResultModuleId
#define R_NAMESPACE_MODULE_ID(nmspc) nmspc::R_CURRENT_NAMESPACE_RESULT_MODULE

#define R_MAKE_NAMESPACE_RESULT(nmspc, desc) static_cast<::ams::Result>(::ams::result::impl::ResultTraits::MakeValue(R_NAMESPACE_MODULE_ID(nmspc), desc))

#define R_DEFINE_ERROR_RESULT_IMPL(name, desc_start, desc_end) \
    class Result##name final : public ::ams::result::impl::ResultErrorBase<R_CURRENT_NAMESPACE_RESULT_MODULE, desc_start>, public ::ams::result::impl::ResultErrorRangeBase<R_CURRENT_NAMESPACE_RESULT_MODULE, desc_start, desc_end> {}

#define R_DEFINE_ABSTRACT_ERROR_RESULT_IMPL(name, desc_start, desc_end) \
    class Result##name final : public ::ams::result::impl::ResultErrorRangeBase<R_CURRENT_NAMESPACE_RESULT_MODULE, desc_start, desc_end> {}


#define R_DEFINE_ERROR_RESULT(name, desc) R_DEFINE_ERROR_RESULT_IMPL(name, desc, desc)
#define R_DEFINE_ERROR_RANGE(name, start, end) R_DEFINE_ERROR_RESULT_IMPL(name, start, end)

#define R_DEFINE_ABSTRACT_ERROR_RESULT(name, desc) R_DEFINE_ABSTRACT_ERROR_RESULT_IMPL(name, desc, desc)
#define R_DEFINE_ABSTRACT_ERROR_RANGE(name, start, end) R_DEFINE_ABSTRACT_ERROR_RESULT_IMPL(name, start, end)

/* Remove libnx macros, replace with our own. */
#ifndef R_SUCCEEDED
#error "R_SUCCEEDED not defined."
#endif

#undef R_SUCCEEDED

#ifndef R_FAILED
#error "R_FAILED not defined"
#endif

#undef R_FAILED

#define R_SUCCEEDED(res) (static_cast<::ams::Result>(res).IsSuccess())
#define R_FAILED(res)    (static_cast<::ams::Result>(res).IsFailure())


/// Evaluates an expression that returns a result, and returns the result if it would fail.
#define R_TRY(res_expr) \
    ({ \
        const auto _tmp_r_try_rc = res_expr; \
        if (R_FAILED(_tmp_r_try_rc)) { \
            return _tmp_r_try_rc; \
        } \
    })

/// Evaluates an expression that returns a result, and fatals the result if it would fail.
#define R_ASSERT(res_expr) \
    ({ \
        const auto _tmp_r_assert_rc = res_expr; \
        if (R_FAILED(_tmp_r_assert_rc)) {  \
            ::ams::result::impl::OnResultAssertion(_tmp_r_assert_rc); \
        } \
    })

/// Evaluates a boolean expression, and returns a result unless that expression is true.
#define R_UNLESS(expr, res) \
    ({ \
        if (!(expr)) { \
            return static_cast<::ams::Result>(res); \
        } \
    })

/// Helpers for pattern-matching on a result expression, if the result would fail.
#define R_CURRENT_RESULT _tmp_r_current_result

#define R_TRY_CATCH(res_expr) \
    ({ \
        const auto R_CURRENT_RESULT = res_expr; \
        if (R_FAILED(R_CURRENT_RESULT)) { \
            if (false)

namespace ams::result::impl {

    template<typename... Rs>
    NX_CONSTEXPR bool AnyIncludes(Result result) {
        return (Rs::Includes(result) || ...);
    }

}

#define R_CATCH(...) \
            } else if (::ams::result::impl::AnyIncludes<__VA_ARGS__>(R_CURRENT_RESULT)) { \
                if (true)

#define R_CONVERT(catch_type, convert_type) \
        R_CATCH(catch_type) { return static_cast<::ams::Result>(convert_type); }

#define R_CATCH_ALL() \
            } else if (R_FAILED(R_CURRENT_RESULT)) { \
                if (true)

#define R_CONVERT_ALL(convert_type) \
        R_CATCH_ALL() { return static_cast<::ams::Result>(convert_type); }

#define R_CATCH_RETHROW(catch_type) \
        R_CONVERT(catch_type, R_CURRENT_RESULT)

#define R_END_TRY_CATCH \
            else if (R_FAILED(R_CURRENT_RESULT)) { \
                return R_CURRENT_RESULT; \
            } \
        } \
    })

#define R_END_TRY_CATCH_WITH_ASSERT \
            else { \
                R_ASSERT(R_CURRENT_RESULT); \
            } \
        } \
    })
