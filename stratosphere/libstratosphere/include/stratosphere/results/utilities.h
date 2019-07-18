/**
 * @file result_utilities.h
 * @brief Utilities for handling Results.
 * @author SciresM
 * @copyright libnx Authors
 */
#pragma once
#include <switch/result.h>

#ifdef __cplusplus
#include <cstdlib>
extern "C" {
#endif

/// Evaluates an expression that returns a result, and returns the result if it would fail.
#define R_TRY(res_expr) \
    ({ \
        const Result _tmp_r_try_rc = res_expr; \
        if (R_FAILED(_tmp_r_try_rc)) { \
            return _tmp_r_try_rc; \
        } \
    })

/// Evaluates an expression that returns a result, and fatals the result if it would fail.
#ifdef RESULT_ABORT_ON_ASSERT
#define R_ASSERT_IMPL(res) std::abort()
#else
#define R_ASSERT_IMPL(res) fatalSimple(res)
#endif

#define R_ASSERT(res_expr) \
    ({ \
        const Result _tmp_r_assert_rc = res_expr; \
        if (R_FAILED(_tmp_r_assert_rc)) {  \
            R_ASSERT_IMPL(_tmp_r_assert_rc); \
        } \
    })

/// Helpers for pattern-matching on a result expression, if the result would fail.
#define R_TRY_CATCH_RESULT _tmp_r_try_catch_rc

#define R_TRY_CATCH(res_expr) \
    ({ \
        const Result R_TRY_CATCH_RESULT = res_expr; \
        if (R_FAILED(R_TRY_CATCH_RESULT)) { \
            if (false)

#define R_CATCH(catch_result) \
            } else if (R_TRY_CATCH_RESULT == catch_result) { \
                _Static_assert(R_FAILED(catch_result), "Catch result must be constexpr error Result!"); \
                if (false) { } \
                else

#define R_GET_CATCH_RANGE_IMPL(_1, _2, NAME, ...) NAME

#define R_CATCH_RANGE_IMPL_2(catch_result_start, catch_result_end) \
            } else if (catch_result_start <= R_TRY_CATCH_RESULT && R_TRY_CATCH_RESULT <= catch_result_end) { \
                _Static_assert(R_FAILED(catch_result_start), "Catch start result must be constexpr error Result!"); \
                _Static_assert(R_FAILED(catch_result_end), "Catch end result must be constexpr error Result!");  \
                _Static_assert(R_MODULE(catch_result_start) == R_MODULE(catch_result_end), "Catch range modules must be equal!"); \
                if (false) { } \
                else

#define R_CATCH_RANGE_IMPL_1(catch_result) R_CATCH_RANGE_IMPL_2(catch_result##RangeStart, catch_result##RangeEnd)

#define R_CATCH_RANGE(...) R_GET_CATCH_RANGE_IMPL(__VA_ARGS__, R_CATCH_RANGE_IMPL_2, R_CATCH_RANGE_IMPL_1)(__VA_ARGS__)

#define R_CATCH_MODULE(module) \
            } else if (R_MODULE(R_TRY_CATCH_RESULT) == module) { \
                _Static_assert(module != 0, "Catch module must be error!"); \
                if (false) { } \
                else

#define R_CATCH_ALL() \
            } else if (R_FAILED(R_TRY_CATCH_RESULT)) { \
                if (false) { } \
                else

#define R_END_TRY_CATCH \
            else if (R_FAILED(R_TRY_CATCH_RESULT)) { \
                return R_TRY_CATCH_RESULT; \
            } \
        } \
    })

/// Evaluates an expression that returns a result, and returns the result (after evaluating a cleanup expression) if it would fail.
#define R_CLEANUP_RESULT _tmp_r_try_cleanup_rc

#define R_TRY_CLEANUP(res_expr, cleanup_expr) \
    ({ \
        const Result R_CLEANUP_RESULT = res_expr; \
        if (R_FAILED(R_CLEANUP_RESULT)) { \
            ({ cleanup_expr }); \
            return R_CLEANUP_RESULT; \
        } \
    })

#ifdef __cplusplus
}
#endif

// For C++, also define R_CATCH_MANY helper.
#ifdef __cplusplus

template<Result... rs>
constexpr inline bool _CheckResultsForMultiTryCatch(const Result &rc) {
    static_assert((R_FAILED(rs) && ...), "Multi try catch result must be constexpr error Result!");
    return ((rs == rc) || ...);
}

#define R_CATCH_MANY(...) \
            } else if (_CheckResultsForMultiTryCatch<__VA_ARGS__>(_tmp_r_try_catch_rc)) { \
                if (false) { } \
                else

#endif
