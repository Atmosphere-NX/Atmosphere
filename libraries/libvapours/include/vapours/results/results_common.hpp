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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>

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
            private:
                static constexpr ALWAYS_INLINE BaseType GetBitsValue(BaseType v, int ofs, int num) {
                    return (v >> ofs) & ~(~BaseType() << num);
                }
            public:
                static constexpr ALWAYS_INLINE BaseType MakeValue(BaseType module, BaseType description) {
                    return (module) | (description << ModuleBits);
                }

                template<BaseType module, BaseType description>
                struct MakeStaticValue : public std::integral_constant<BaseType, MakeValue(module, description)> {
                    static_assert(module < (1 << ModuleBits), "Invalid Module");
                    static_assert(description < (1 << DescriptionBits), "Invalid Description");
                };

                static constexpr ALWAYS_INLINE BaseType GetModuleFromValue(BaseType value) {
                    return GetBitsValue(value, 0, ModuleBits);
                }

                static constexpr ALWAYS_INLINE BaseType GetDescriptionFromValue(BaseType value) {
                    return GetBitsValue(value, ModuleBits, DescriptionBits);
                }

                static constexpr ALWAYS_INLINE BaseType GetReservedFromValue(BaseType value) {
                    return GetBitsValue(value, ModuleBits + DescriptionBits, ReservedBits);
                }

                static constexpr ALWAYS_INLINE BaseType MaskReservedFromValue(BaseType value) {
                    return value & ~(~(~BaseType() << ReservedBits) << (ModuleBits + DescriptionBits));
                }

                static constexpr ALWAYS_INLINE BaseType MergeValueWithReserved(BaseType value, BaseType reserved) {
                    return (value << 0) | (reserved << (ModuleBits + DescriptionBits));
                }
        };

        /* Use CRTP for Results. */
        template<typename Self>
        class ResultBase {
            public:
                using BaseType = typename ResultTraits::BaseType;
                static constexpr BaseType SuccessValue = ResultTraits::SuccessValue;
            public:
                constexpr ALWAYS_INLINE BaseType GetModule() const { return ResultTraits::GetModuleFromValue(static_cast<const Self *>(this)->GetValue()); }
                constexpr ALWAYS_INLINE BaseType GetDescription() const { return ResultTraits::GetDescriptionFromValue(static_cast<const Self *>(this)->GetValue()); }
        };

        class ResultInternalAccessor;

    }

    class ResultSuccess;

    class Result final : public result::impl::ResultBase<Result> {
        friend class result::impl::ResultInternalAccessor;
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

            constexpr ALWAYS_INLINE operator ResultSuccess() const;
            NX_CONSTEXPR bool CanAccept(Result) { return true; }

            constexpr ALWAYS_INLINE bool IsSuccess() const { return this->GetValue() == Base::SuccessValue; }
            constexpr ALWAYS_INLINE bool IsFailure() const { return !this->IsSuccess(); }
            constexpr ALWAYS_INLINE typename Base::BaseType GetModule() const { return Base::GetModule(); }
            constexpr ALWAYS_INLINE typename Base::BaseType GetDescription() const { return Base::GetDescription(); }

            constexpr ALWAYS_INLINE typename Base::BaseType GetValue() const { return this->value; }
    };
    static_assert(sizeof(Result) == sizeof(Result::Base::BaseType), "sizeof(Result) == sizeof(Result::Base::BaseType)");
    static_assert(std::is_trivially_destructible<Result>::value, "std::is_trivially_destructible<Result>::value");

    namespace result::impl {

        class ResultInternalAccessor {
            public:
                static constexpr ALWAYS_INLINE Result MakeResult(ResultTraits::BaseType value) {
                    return Result(value);
                }

                static constexpr ALWAYS_INLINE ResultTraits::BaseType GetReserved(Result result) {
                    return ResultTraits::GetReservedFromValue(result.value);
                }

                static constexpr ALWAYS_INLINE Result MergeReserved(Result result, ResultTraits::BaseType reserved) {
                    return Result(ResultTraits::MergeValueWithReserved(ResultTraits::MaskReservedFromValue(result.value), reserved));
                }
        };

        constexpr ALWAYS_INLINE Result MakeResult(ResultTraits::BaseType value) {
            return ResultInternalAccessor::MakeResult(value);
        }

    }

    class ResultSuccess final : public result::impl::ResultBase<ResultSuccess> {
        public:
            using Base = typename result::impl::ResultBase<ResultSuccess>;
        public:
            constexpr operator Result() const { return result::impl::MakeResult(Base::SuccessValue); }
            NX_CONSTEXPR bool CanAccept(Result result) { return result.IsSuccess(); }

            constexpr ALWAYS_INLINE bool IsSuccess() const { return true; }
            constexpr ALWAYS_INLINE bool IsFailure() const { return !this->IsSuccess(); }

            constexpr ALWAYS_INLINE typename Base::BaseType GetValue() const { return Base::SuccessValue; }
    };

    namespace result::impl {

        NORETURN NOINLINE void OnResultAssertion(const char *file, int line, const char *func, const char *expr, Result result);
        NORETURN NOINLINE void OnResultAssertion(Result result);
        NORETURN NOINLINE void OnResultAbort(const char *file, int line, const char *func, const char *expr, Result result);
        NORETURN NOINLINE void OnResultAbort(Result result);

    }

    constexpr ALWAYS_INLINE Result::operator ResultSuccess() const {
        if (!ResultSuccess::CanAccept(*this)) {
            result::impl::OnResultAbort(*this);
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
                constexpr operator ResultSuccess() const { OnResultAbort(Value); }

                constexpr ALWAYS_INLINE bool IsSuccess() const { return false; }
                constexpr ALWAYS_INLINE bool IsFailure() const { return !this->IsSuccess(); }

                constexpr ALWAYS_INLINE typename Base::BaseType GetValue() const { return Value; }
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
                static constexpr bool Includes(Result result) {
                    return result.GetModule() == Module && DescriptionStart <= result.GetDescription() && result.GetDescription() <= DescriptionEnd;
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
        const auto _tmp_r_try_rc = (res_expr); \
        if (R_FAILED(_tmp_r_try_rc)) { \
            return _tmp_r_try_rc; \
        } \
    })

#ifdef AMS_ENABLE_DEBUG_PRINT
#define AMS_CALL_ON_RESULT_ASSERTION_IMPL(cond, val) ::ams::result::impl::OnResultAssertion(__FILE__, __LINE__, __PRETTY_FUNCTION__, cond, val)
#define AMS_CALL_ON_RESULT_ABORT_IMPL(cond, val)  ::ams::result::impl::OnResultAbort(__FILE__, __LINE__, __PRETTY_FUNCTION__, cond, val)
#else
#define AMS_CALL_ON_RESULT_ASSERTION_IMPL(cond, val) ::ams::result::impl::OnResultAssertion("", 0, "", "", val)
#define AMS_CALL_ON_RESULT_ABORT_IMPL(cond, val)  ::ams::result::impl::OnResultAbort("", 0, "", "", val)
#endif

/// Evaluates an expression that returns a result, and asserts the result if it would fail.
#ifdef AMS_ENABLE_ASSERTIONS
#define R_ASSERT(res_expr) \
    ({ \
        const auto _tmp_r_assert_rc = (res_expr); \
        if (AMS_UNLIKELY(R_FAILED(_tmp_r_assert_rc))) {  \
            AMS_CALL_ON_RESULT_ASSERTION_IMPL(#res_expr, _tmp_r_assert_rc); \
        } \
    })
#else
#define R_ASSERT(res_expr) AMS_UNUSED((res_expr));
#endif

/// Evaluates an expression that returns a result, and aborts if the result would fail.
#define R_ABORT_UNLESS(res_expr) \
    ({ \
        const auto _tmp_r_abort_rc = (res_expr); \
        if (AMS_UNLIKELY(R_FAILED(_tmp_r_abort_rc))) {  \
            AMS_CALL_ON_RESULT_ABORT_IMPL(#res_expr, _tmp_r_abort_rc); \
        } \
    })

/// Evaluates a boolean expression, and returns a result unless that expression is true.
#define R_UNLESS(expr, res) \
    ({ \
        if (!(expr)) { \
            return static_cast<::ams::Result>(res); \
        } \
    })

/// Evaluates a boolean expression, and succeeds if that expression is true.
#define R_SUCCEED_IF(expr) R_UNLESS(!(expr), ResultSuccess())

/// Helpers for pattern-matching on a result expression, if the result would fail.
#define R_CURRENT_RESULT _tmp_r_current_result

#define R_TRY_CATCH(res_expr) \
    ({ \
        const auto R_CURRENT_RESULT = (res_expr); \
        if (R_FAILED(R_CURRENT_RESULT)) { \
            if (false)

namespace ams::result::impl {

    template<typename... Rs>
    constexpr ALWAYS_INLINE bool AnyIncludes(Result result) {
        return (Rs::Includes(result) || ...);
    }

}

#define R_CATCH(...) \
            } else if (::ams::result::impl::AnyIncludes<__VA_ARGS__>(R_CURRENT_RESULT)) { \
                if (true)

#define R_CATCH_MODULE(__module__) \
            } else if ((R_CURRENT_RESULT).GetModule() == ::ams::R_NAMESPACE_MODULE_ID(__module__)) { \
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


#define R_END_TRY_CATCH_WITH_ABORT_UNLESS \
            else { \
                R_ABORT_UNLESS(R_CURRENT_RESULT); \
            } \
        } \
    })
