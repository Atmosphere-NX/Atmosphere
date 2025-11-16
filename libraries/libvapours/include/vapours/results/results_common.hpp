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

namespace ams {

    const char *GetResultName(int module, int description);

    namespace result::impl {

        #if defined(AMS_AUTO_GENERATE_RESULT_NAMES)
        struct DummyNameHolder {
            static constexpr bool Exists = false;
            static constexpr const char *Name = "unknown";
        };

        template<int Module>
        struct ResultNameSpaceExistsImpl {
            static constexpr bool Exists = false;

            template<int Description>
            using NameHolder = DummyNameHolder;
        };
        #endif

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
                [[nodiscard]] static constexpr ALWAYS_INLINE BaseType MakeValue(BaseType module, BaseType description) {
                    return (module) | (description << ModuleBits);
                }

                template<BaseType module, BaseType description>
                struct MakeStaticValue : public std::integral_constant<BaseType, MakeValue(module, description)> {
                    static_assert(module < (1 << ModuleBits), "Invalid Module");
                    static_assert(description < (1 << DescriptionBits), "Invalid Description");
                };

                [[nodiscard]] static constexpr ALWAYS_INLINE BaseType GetModuleFromValue(BaseType value) {
                    return GetBitsValue(value, 0, ModuleBits);
                }

                [[nodiscard]] static constexpr ALWAYS_INLINE BaseType GetDescriptionFromValue(BaseType value) {
                    return GetBitsValue(value, ModuleBits, DescriptionBits);
                }

                [[nodiscard]] static constexpr ALWAYS_INLINE BaseType GetReservedFromValue(BaseType value) {
                    return GetBitsValue(value, ModuleBits + DescriptionBits, ReservedBits);
                }

                [[nodiscard]] static constexpr ALWAYS_INLINE BaseType MaskReservedFromValue(BaseType value) {
                    return value & ~(~(~BaseType() << ReservedBits) << (ModuleBits + DescriptionBits));
                }

                [[nodiscard]] static constexpr ALWAYS_INLINE BaseType MergeValueWithReserved(BaseType value, BaseType reserved) {
                    return (value << 0) | (reserved << (ModuleBits + DescriptionBits));
                }
        };

        /* Use CRTP for Results. */
        class ResultBase {
            public:
                using BaseType = typename ResultTraits::BaseType;
                static constexpr BaseType SuccessValue = ResultTraits::SuccessValue;
            public:
                [[nodiscard]] constexpr ALWAYS_INLINE BaseType GetModule(this auto const &self) { return ResultTraits::GetModuleFromValue(self.GetValue()); }
                [[nodiscard]] constexpr ALWAYS_INLINE BaseType GetDescription(this auto const &self) { return ResultTraits::GetDescriptionFromValue(self.GetValue()); }
        };

        class ResultInternalAccessor;

    }

    class ResultSuccess;

    class [[nodiscard]] Result final : public result::impl::ResultBase {
        friend class result::impl::ResultInternalAccessor;
        public:
            using Base = typename result::impl::ResultBase;
        private:
            typename Base::BaseType m_value;
        private:
            /* TODO: Maybe one-day, the result constructor. */
        public:
            Result() { /* ... */ }

            /* TODO: It sure would be nice to make this private. */
            constexpr ALWAYS_INLINE Result(typename Base::BaseType v) : m_value(v) { static_assert(std::is_same<typename Base::BaseType, ::Result>::value); }

            constexpr ALWAYS_INLINE operator ResultSuccess() const;

            [[nodiscard]] static constexpr ALWAYS_INLINE bool CanAccept(Result) { return true; }

            [[nodiscard]] constexpr ALWAYS_INLINE bool IsSuccess() const { return m_value == Base::SuccessValue; }
            [[nodiscard]] constexpr ALWAYS_INLINE bool IsFailure() const { return !this->IsSuccess(); }
            [[nodiscard]] constexpr ALWAYS_INLINE typename Base::BaseType GetModule() const { return Base::GetModule(); }
            [[nodiscard]] constexpr ALWAYS_INLINE typename Base::BaseType GetDescription() const { return Base::GetDescription(); }

            [[nodiscard]] constexpr ALWAYS_INLINE typename Base::BaseType GetInnerValue() const { return ::ams::result::impl::ResultTraits::MaskReservedFromValue(m_value); }

            [[nodiscard]] constexpr ALWAYS_INLINE typename Base::BaseType GetValue() const { return m_value; }
    };
    static_assert(sizeof(Result) == sizeof(Result::Base::BaseType), "sizeof(Result) == sizeof(Result::Base::BaseType)");
    static_assert(std::is_trivially_destructible<Result>::value, "std::is_trivially_destructible<Result>::value");

    ALWAYS_INLINE const char *GetResultName(const Result &result) {
        return GetResultName(result.GetModule(), result.GetDescription());
    }

    namespace result::impl {

        class ResultInternalAccessor {
            public:
                [[nodiscard]] static constexpr ALWAYS_INLINE Result MakeResult(ResultTraits::BaseType value) {
                    return Result(value);
                }

                [[nodiscard]] static constexpr ALWAYS_INLINE ResultTraits::BaseType GetReserved(Result result) {
                    return ResultTraits::GetReservedFromValue(result.m_value);
                }

                [[nodiscard]] static constexpr ALWAYS_INLINE Result MergeReserved(Result result, ResultTraits::BaseType reserved) {
                    return Result(ResultTraits::MergeValueWithReserved(ResultTraits::MaskReservedFromValue(result.m_value), reserved));
                }
        };

        [[nodiscard]] constexpr ALWAYS_INLINE Result MakeResult(ResultTraits::BaseType value) {
            return ResultInternalAccessor::MakeResult(value);
        }

    }

    class ResultSuccess final : public result::impl::ResultBase {
        public:
            using Base = typename result::impl::ResultBase;
        public:
            constexpr ALWAYS_INLINE operator Result() const { return result::impl::MakeResult(Base::SuccessValue); }
            [[nodiscard]] static constexpr ALWAYS_INLINE bool CanAccept(Result result) { return result.IsSuccess(); }

            [[nodiscard]] constexpr ALWAYS_INLINE bool IsSuccess() const { return true; }
            [[nodiscard]] constexpr ALWAYS_INLINE bool IsFailure() const { return !this->IsSuccess(); }

            [[nodiscard]] constexpr ALWAYS_INLINE typename Base::BaseType GetValue() const { return Base::SuccessValue; }
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
        class ResultErrorBase : public ResultBase {
            public:
                using Base = typename result::impl::ResultBase;
                static constexpr typename Base::BaseType Module = _Module;
                static constexpr typename Base::BaseType Description = _Description;
                static constexpr typename Base::BaseType Value = ResultTraits::MakeStaticValue<Module, Description>::value;
                static_assert(Value != Base::SuccessValue, "Value != Base::SuccessValue");
            public:
                constexpr ALWAYS_INLINE operator Result() const { return MakeResult(Value); }
                constexpr ALWAYS_INLINE operator ResultSuccess() const {
                    OnResultAbort(Value);
                    __builtin_unreachable();
                    return ResultSuccess();
                }

                [[nodiscard]] constexpr ALWAYS_INLINE bool IsSuccess() const { return false; }
                [[nodiscard]] constexpr ALWAYS_INLINE bool IsFailure() const { return !this->IsSuccess(); }

                [[nodiscard]] constexpr ALWAYS_INLINE typename Base::BaseType GetValue() const { return Value; }
        };

        template<ResultTraits::BaseType _Module, ResultTraits::BaseType DescStart, ResultTraits::BaseType DescEnd>
        class ResultErrorRangeBase {
            private:
                /* NOTE: GCC does not optimize the module/description comparisons into one check (as of 10/1/2021) */
                /*       and so this optimizes result comparisons to get the same codegen as Nintendo does. */
                static constexpr bool UseDirectValueComparison = true;
            public:
                static constexpr ResultTraits::BaseType Module = _Module;
                static constexpr ResultTraits::BaseType DescriptionStart = DescStart;
                static constexpr ResultTraits::BaseType DescriptionEnd = DescEnd;
                static_assert(DescriptionStart <= DescriptionEnd, "DescriptionStart <= DescriptionEnd");
                static constexpr typename ResultTraits::BaseType StartValue = ResultTraits::MakeStaticValue<Module, DescriptionStart>::value;
                static constexpr typename ResultTraits::BaseType EndValue = ResultTraits::MakeStaticValue<Module, DescriptionEnd>::value;
            public:
                [[nodiscard]] static constexpr ALWAYS_INLINE bool Includes(Result result) {
                    if constexpr (UseDirectValueComparison) {
                        const auto inner_value = result.GetInnerValue();
                        if constexpr (StartValue == EndValue) {
                            return inner_value == StartValue;
                        } else {
                            return StartValue <= inner_value && inner_value <= EndValue;
                        }
                    } else {
                        return result.GetModule() == Module && DescriptionStart <= result.GetDescription() && result.GetDescription() <= DescriptionEnd;
                    }
                }
        };

    }

    #if defined(ATMOSPHERE_BOARD_NINTENDO_NX) && defined(ATMOSPHERE_ARCH_ARM64) && defined(ATMOSPHERE_IS_STRATOSPHERE)
    namespace diag::impl {

        void FatalErrorByResultForNx(Result result) noexcept NORETURN;

    }
    #endif

}

/* Macros for defining new results. */
#if defined(AMS_AUTO_GENERATE_RESULT_NAMES)
#define R_DEFINE_NAMESPACE_RESULT_MODULE(nmspc, value)                                                  \
    namespace nmspc {                                                                                   \
                                                                                                        \
        namespace result_impl {                                                                         \
            static constexpr inline ::ams::result::impl::ResultTraits::BaseType ResultModuleId = value; \
                                                                                                        \
            template<int Description>                                                                   \
            struct ResultNameHolderImpl { static constexpr bool Exists = false; };                      \
        }                                                                                               \
                                                                                                        \
    }                                                                                                   \
                                                                                                        \
    namespace ams::result::impl {                                                                       \
                                                                                                        \
        template<> struct ResultNameSpaceExistsImpl<value> {                                            \
            static constexpr bool Exists = true;                                                        \
                                                                                                        \
            template<int Description>                                                                   \
            using NameHolder = nmspc::result_impl::ResultNameHolderImpl<Description>;                   \
        };                                                                                              \
                                                                                                        \
    }
#else
#define R_DEFINE_NAMESPACE_RESULT_MODULE(nmspc, value)                                                  \
    namespace nmspc {                                                                                   \
                                                                                                        \
        namespace result_impl {                                                                         \
            static constexpr inline ::ams::result::impl::ResultTraits::BaseType ResultModuleId = value; \
        }                                                                                               \
                                                                                                        \
    }
#endif

#define R_CURRENT_NAMESPACE_RESULT_MODULE result_impl::ResultModuleId
#define R_NAMESPACE_MODULE_ID(nmspc) nmspc::R_CURRENT_NAMESPACE_RESULT_MODULE

#define R_MAKE_NAMESPACE_RESULT(nmspc, desc) static_cast<::ams::Result>(::ams::result::impl::ResultTraits::MakeValue(R_NAMESPACE_MODULE_ID(nmspc), desc))

#if defined(AMS_AUTO_GENERATE_RESULT_NAMES)
#define R_DEFINE_ERROR_RESULT_NAME_HOLDER_IMPL(name, desc_start, desc_end) \
    template<> struct result_impl::ResultNameHolderImpl<desc_start> { static constexpr bool Exists = true; static constexpr const char *Name = #name; };
#else
#define R_DEFINE_ERROR_RESULT_NAME_HOLDER_IMPL(name, desc_start, desc_end)
#endif

#define R_DEFINE_ERROR_RESULT_CLASS_IMPL(name, desc_start, desc_end) \
    class Result##name final : public ::ams::result::impl::ResultErrorBase<R_CURRENT_NAMESPACE_RESULT_MODULE, desc_start>, public ::ams::result::impl::ResultErrorRangeBase<R_CURRENT_NAMESPACE_RESULT_MODULE, desc_start, desc_end> {}

#define R_DEFINE_ERROR_RESULT_IMPL(name, desc_start, desc_end)         \
    R_DEFINE_ERROR_RESULT_NAME_HOLDER_IMPL(name, desc_start, desc_end) \
    R_DEFINE_ERROR_RESULT_CLASS_IMPL(name, desc_start, desc_end)

#define R_DEFINE_ABSTRACT_ERROR_RESULT_IMPL(name, desc_start, desc_end) \
    class Result##name final : public ::ams::result::impl::ResultErrorRangeBase<R_CURRENT_NAMESPACE_RESULT_MODULE, desc_start, desc_end> {}

#define R_DEFINE_ERROR_RESULT(name, desc) R_DEFINE_ERROR_RESULT_IMPL(name, desc, desc)
#define R_DEFINE_ERROR_RANGE(name, start, end) R_DEFINE_ERROR_RESULT_IMPL(name, start, end)

#define R_DEFINE_ABSTRACT_ERROR_RESULT(name, desc) R_DEFINE_ABSTRACT_ERROR_RESULT_IMPL(name, desc, desc)
#define R_DEFINE_ABSTRACT_ERROR_RANGE(name, start, end) R_DEFINE_ABSTRACT_ERROR_RESULT_IMPL(name, start, end)


#define R_DEFINE_ERROR_RESULT_NS(ns, name, desc) namespace ns { R_DEFINE_ERROR_RESULT_CLASS_IMPL(name, desc, desc); } R_DEFINE_ERROR_RESULT_NAME_HOLDER_IMPL(name, desc, desc)
#define R_DEFINE_ERROR_RANGE_NS(ns, name, start, end) namespace ns { R_DEFINE_ERROR_RESULT_CLASS_IMPL(name, start, end); } R_DEFINE_ERROR_RESULT_NAME_HOLDER_IMPL(name, start, end)

#define R_DEFINE_ABSTRACT_ERROR_RESULT_NS(ns, name, desc) namespace ns {  R_DEFINE_ABSTRACT_ERROR_RESULT_IMPL(name, desc, desc); }
#define R_DEFINE_ABSTRACT_ERROR_RANGE_NS(ns, name, start, end) namespace ns { R_DEFINE_ABSTRACT_ERROR_RESULT_IMPL(name, start, end); }

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


/* NOTE: The following are experimental and cannot be safely used yet. */
/* =================================================================== */
constinit inline ::ams::Result __TmpCurrentResultReference = ::ams::ResultSuccess();

namespace ams::result::impl {

    template<auto EvaluateResult, class F>
    class ScopedResultGuard {
        NON_COPYABLE(ScopedResultGuard);
        NON_MOVEABLE(ScopedResultGuard);
        private:
            Result &m_ref;
            F m_f;
        public:
            constexpr ALWAYS_INLINE ScopedResultGuard(Result &ref, F f) : m_ref(ref), m_f(std::move(f)) { }
            constexpr ALWAYS_INLINE ~ScopedResultGuard() { if (EvaluateResult(m_ref)) { m_f(); } }
    };

    template<auto EvaluateResult>
    class ResultReferenceForScopedResultGuard {
        private:
            Result &m_ref;
        public:
            constexpr ALWAYS_INLINE ResultReferenceForScopedResultGuard(Result &r) : m_ref(r) { /* ... */ }

            constexpr ALWAYS_INLINE operator Result &() const { return m_ref; }
    };

    template<auto EvaluateResult, typename F>
    constexpr ALWAYS_INLINE ScopedResultGuard<EvaluateResult, F> operator+(ResultReferenceForScopedResultGuard<EvaluateResult> ref, F&& f) {
        return ScopedResultGuard<EvaluateResult, F>(static_cast<Result &>(ref), std::forward<F>(f));
    }

    constexpr ALWAYS_INLINE bool EvaluateResultSuccess(const ::ams::Result &r) { return R_SUCCEEDED(r); }
    constexpr ALWAYS_INLINE bool EvaluateResultFailure(const ::ams::Result &r) { return R_FAILED(r); }

    template<typename R>
    constexpr ALWAYS_INLINE bool EvaluateResultIncludedImplForSuccessCompatibility(const ::ams::Result &r) {
        if constexpr (std::same_as<R, ::ams::ResultSuccess>) {
            return R_SUCCEEDED(r);
        } else {
            return R::Includes(r);
        }
    }

    template<typename... Rs>
    constexpr ALWAYS_INLINE bool EvaluateAnyResultIncludes(const ::ams::Result &r) { return (EvaluateResultIncludedImplForSuccessCompatibility<Rs>(r) || ...); }

    template<typename... Rs>
    constexpr ALWAYS_INLINE bool EvaluateResultNotIncluded(const ::ams::Result &r) { return !EvaluateAnyResultIncludes<Rs...>(r); }

}

#define AMS_DECLARE_CURRENT_RESULT_REFERENCE_AND_STORAGE(COUNTER_VALUE)                                                              \
    [[maybe_unused]] constexpr bool HasPrevRef_##COUNTER_VALUE = std::same_as<decltype(__TmpCurrentResultReference), Result &>;      \
    [[maybe_unused]] auto &PrevRef_##COUNTER_VALUE = __TmpCurrentResultReference;                                                    \
    [[maybe_unused]] Result __tmp_result_##COUNTER_VALUE = ResultSuccess();                                                          \
    ::ams::Result &__TmpCurrentResultReference = HasPrevRef_##COUNTER_VALUE ? PrevRef_##COUNTER_VALUE : __tmp_result_##COUNTER_VALUE

#define ON_RESULT_RETURN_IMPL(...)                                                                                                                                                 \
    static_assert(std::same_as<decltype(__TmpCurrentResultReference), Result &>);                                                                                                  \
    auto ANONYMOUS_VARIABLE(RESULT_GUARD_STATE_) = ::ams::result::impl::ResultReferenceForScopedResultGuard<__VA_ARGS__>(__TmpCurrentResultReference) + [&]() ALWAYS_INLINE_LAMBDA

#define ON_RESULT_FAILURE_2 ON_RESULT_RETURN_IMPL(::ams::result::impl::EvaluateResultFailure)

#define ON_RESULT_FAILURE                                                                                                                                                   \
    AMS_DECLARE_CURRENT_RESULT_REFERENCE_AND_STORAGE(__COUNTER__);                                                                                                          \
    ON_RESULT_FAILURE_2

#define ON_RESULT_SUCCESS_2 ON_RESULT_RETURN_IMPL(::ams::result::impl::EvaluateResultSuccess)

#define ON_RESULT_SUCCESS                                                                                                                                                    \
    AMS_DECLARE_CURRENT_RESULT_REFERENCE_AND_STORAGE(__COUNTER__);                                                                                                           \
    ON_RESULT_SUCCESS_2

#define ON_RESULT_INCLUDED_2(...) ON_RESULT_RETURN_IMPL(::ams::result::impl::EvaluateAnyResultIncludes<__VA_ARGS__>)

#define ON_RESULT_INCLUDED(...)                                                                                                                                              \
    AMS_DECLARE_CURRENT_RESULT_REFERENCE_AND_STORAGE(__COUNTER__);                                                                                                           \
    ON_RESULT_INCLUDED_2(__VA_ARGS__)

#define ON_RESULT_NOT_INCLUDED_2(...) ON_RESULT_RETURN_IMPL(::ams::result::impl::EvaluateResultNotIncluded<__VA_ARGS__>)

#define ON_RESULT_NOT_INCLUDED(...)                                                                                                                                          \
    AMS_DECLARE_CURRENT_RESULT_REFERENCE_AND_STORAGE(__COUNTER__);                                                                                                           \
    ON_RESULT_NOT_INCLUDED_2(__VA_ARGS__)

#define ON_RESULT_FAILURE_BESIDES(...)   ON_RESULT_NOT_INCLUDED(::ams::ResultSuccess, ## __VA_ARGS__)

#define ON_RESULT_FAILURE_BESIDES_2(...) ON_RESULT_NOT_INCLUDED_2(::ams::ResultSuccess, ## __VA_ARGS__)

/* =================================================================== */

/// Returns a result.
#define R_RETURN(res_expr)                                                                                                                      \
    {                                                                                                                                          \
        const ::ams::Result _tmp_r_throw_rc = (res_expr);                                                                                      \
        if constexpr (std::same_as<decltype(__TmpCurrentResultReference), ::ams::Result &>) { __TmpCurrentResultReference = _tmp_r_throw_rc; } \
        return _tmp_r_throw_rc;                                                                                                                \
    }

/// Returns ResultSuccess()
#define R_SUCCEED() R_RETURN(::ams::ResultSuccess())

/// Throws a result.
#define R_THROW(res_expr) R_RETURN(res_expr)

/// Evaluates an expression that returns a result, and returns the result if it would fail.
#define R_TRY(res_expr)                                                       \
    {                                                                         \
        if (const auto _tmp_r_try_rc = (res_expr); R_FAILED(_tmp_r_try_rc)) { \
            R_THROW(_tmp_r_try_rc);                                           \
        }                                                                     \
    }

#if defined(ATMOSPHERE_BOARD_NINTENDO_NX) && defined(ATMOSPHERE_IS_STRATOSPHERE) && !defined(AMS_ENABLE_DETAILED_ASSERTIONS) && !defined(AMS_BUILD_FOR_DEBUGGING) && !defined(AMS_BUILD_FOR_AUDITING)
    #define AMS_CALL_ON_RESULT_ASSERTION_IMPL(cond, val) do { ::ams::diag::impl::FatalErrorByResultForNx(val); AMS_INFINITE_LOOP(); AMS_ASSUME(false); } while (false)
    #define AMS_CALL_ON_RESULT_ABORT_IMPL(cond, val) do { ::ams::diag::impl::FatalErrorByResultForNx(val); AMS_INFINITE_LOOP(); AMS_ASSUME(false); } while (false)
#elif defined(ATMOSPHERE_OS_HORIZON)
    #define AMS_CALL_ON_RESULT_ASSERTION_IMPL(cond, val) AMS_CALL_ASSERT_FAIL_IMPL(::ams::diag::AssertionType_Assert, "ams::Result::IsSuccess()", "Failed: %s\n  Module:      %d\n  Description: %d\n  InnerValue:  0x%08" PRIX32, cond, val.GetModule(), val.GetDescription(), static_cast<::ams::Result>(val).GetInnerValue())
    #define AMS_CALL_ON_RESULT_ABORT_IMPL(cond, val)  AMS_CALL_ABORT_IMPL("ams::Result::IsSuccess()", "Failed: %s\n  Module:      %d\n  Description: %d\n  InnerValue:  0x%08" PRIX32, cond, static_cast<::ams::Result>(val).GetModule(), static_cast<::ams::Result>(val).GetDescription(), static_cast<::ams::Result>(val).GetInnerValue())
#else
    #define AMS_CALL_ON_RESULT_ASSERTION_IMPL(cond, val) AMS_CALL_ASSERT_FAIL_IMPL(::ams::diag::AssertionType_Assert, "ams::Result::IsSuccess()", "Failed: %s\n  Module:      %d\n  Description: %d\n  InnerValue:  0x%08" PRIX32 "\n  Name:        %s", cond, val.GetModule(), val.GetDescription(), static_cast<::ams::Result>(val).GetInnerValue(), ::ams::GetResultName(static_cast<::ams::Result>(val)))
    #define AMS_CALL_ON_RESULT_ABORT_IMPL(cond, val)  AMS_CALL_ABORT_IMPL("ams::Result::IsSuccess()", "Failed: %s\n  Module:      %d\n  Description: %d\n  InnerValue:  0x%08" PRIX32  "\n  Name:        %s", cond, static_cast<::ams::Result>(val).GetModule(), static_cast<::ams::Result>(val).GetDescription(), static_cast<::ams::Result>(val).GetInnerValue(), ::ams::GetResultName(static_cast<::ams::Result>(val)))
#endif

/// Evaluates an expression that returns a result, and asserts the result if it would fail.
#ifdef AMS_ENABLE_ASSERTIONS
#define R_ASSERT(res_expr)                                                                        \
    {                                                                                             \
        if (const auto _tmp_r_assert_rc = (res_expr); AMS_UNLIKELY(R_FAILED(_tmp_r_assert_rc))) { \
            AMS_CALL_ON_RESULT_ASSERTION_IMPL(#res_expr, _tmp_r_assert_rc);                       \
        }                                                                                         \
    }
#else
#define R_ASSERT(res_expr) AMS_UNUSED((res_expr));
#endif

/// Evaluates an expression that returns a result, and aborts if the result would fail.
#define R_ABORT_UNLESS(res_expr)                                                                \
    {                                                                                           \
        if (const auto _tmp_r_abort_rc = (res_expr); AMS_UNLIKELY(R_FAILED(_tmp_r_abort_rc))) { \
            AMS_CALL_ON_RESULT_ABORT_IMPL(#res_expr, _tmp_r_abort_rc);                          \
        }                                                                                       \
    }

/// Evaluates a boolean expression, and returns a result unless that expression is true.
#define R_UNLESS(expr, res) \
    {                       \
        if (!(expr)) {      \
            R_THROW(res);   \
        }                   \
    }

/// Evaluates a boolean expression, and succeeds if that expression is true.
#define R_SUCCEED_IF(expr) R_UNLESS(!(expr), ResultSuccess())

/// Helpers for pattern-matching on a result expression, if the result would fail.
#define R_CURRENT_RESULT _tmp_r_try_catch_current_result

#define R_TRY_CATCH(res_expr)                     \
     {                                            \
        const auto R_CURRENT_RESULT = (res_expr); \
        if (R_FAILED(R_CURRENT_RESULT)) {         \
            if (false)

#define R_CATCH(...)                                                                                    \
            } else if (::ams::result::impl::EvaluateAnyResultIncludes<__VA_ARGS__>(R_CURRENT_RESULT)) { \
                if (true)

#define R_CATCH_MODULE(__module__)                                                                   \
            } else if ((R_CURRENT_RESULT).GetModule() == ::ams::R_NAMESPACE_MODULE_ID(__module__)) { \
                if (true)

#define R_CONVERT(catch_type, convert_type) \
        R_CATCH(catch_type) { R_THROW(static_cast<::ams::Result>(convert_type)); }

#define R_CATCH_ALL()                                \
            } else if (R_FAILED(R_CURRENT_RESULT)) { \
                if (true)

#define R_CONVERT_ALL(convert_type) \
        R_CATCH_ALL() { R_THROW(static_cast<::ams::Result>(convert_type)); }

#define R_CATCH_RETHROW(catch_type) \
        R_CONVERT(catch_type, R_CURRENT_RESULT)

#define R_END_TRY_CATCH                            \
            else if (R_FAILED(R_CURRENT_RESULT)) { \
                R_THROW(R_CURRENT_RESULT);         \
            }                                      \
        }                                          \
    }

#define R_END_TRY_CATCH_WITH_ASSERT         \
            else {                          \
                R_ASSERT(R_CURRENT_RESULT); \
            }                               \
        }                                   \
    }


#define R_END_TRY_CATCH_WITH_ABORT_UNLESS         \
            else {                                \
                R_ABORT_UNLESS(R_CURRENT_RESULT); \
            }                                     \
        }                                         \
    }


