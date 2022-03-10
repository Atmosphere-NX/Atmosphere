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

namespace ams {

    class Result;

    namespace os {

        struct UserExceptionInfo;

    }

    namespace impl {

        NORETURN void UnexpectedDefaultImpl(const char *func, const char *file, int line);

    }

}

namespace ams::diag {

    enum AssertionType {
        AssertionType_Audit,
        AssertionType_Assert,
    };

    struct LogMessage;

    struct AssertionInfo {
        AssertionType type;
        const LogMessage *message;
        const char *expr;
        const char *func;
        const char *file;
        int line;
    };

    enum AbortReason {
        AbortReason_Audit,
        AbortReason_Assert,
        AbortReason_Abort,
        AbortReason_UnexpectedDefault,
    };

    struct AbortInfo {
        AbortReason reason;
        const LogMessage *message;
        const char *expr;
        const char *func;
        const char *file;
        int line;
    };

    void OnAssertionFailure(AssertionType type, const char *expr, const char *func, const char *file, int line, const char *format, ...) __attribute__((format(printf, 6, 7)));
    void OnAssertionFailure(AssertionType type, const char *expr, const char *func, const char *file, int line);

    NORETURN void AbortImpl(const char *expr, const char *func, const char *file, int line);
    NORETURN void AbortImpl(const char *expr, const char *func, const char *file, int line, const char *format, ...) __attribute__((format(printf, 5, 6)));
    NORETURN void AbortImpl(const char *expr, const char *func, const char *file, int line, const ::ams::Result *result, const char *format, ...) __attribute__((format(printf, 6, 7)));

    NORETURN void AbortImpl(const char *expr, const char *func, const char *file, int line, const ::ams::Result *result, const ::ams::os::UserExceptionInfo *exception_info, const char *fmt, ...) __attribute__((format(printf, 7, 8)));

    NORETURN void VAbortImpl(const char *expr, const char *func, const char *file, int line, const ::ams::Result *result, const ::ams::os::UserExceptionInfo *exception_info, const char *fmt, std::va_list vl);

}

#ifdef AMS_ENABLE_DETAILED_ASSERTIONS
#define AMS_CALL_ASSERT_FAIL_IMPL(type, expr, ...) ::ams::diag::OnAssertionFailure(type, expr, __PRETTY_FUNCTION__, __FILE__, __LINE__, ## __VA_ARGS__)
#define AMS_CALL_ABORT_IMPL(expr, ...)       ::ams::diag::AbortImpl(expr, __PRETTY_FUNCTION__, __FILE__, __LINE__, ## __VA_ARGS__)
#define AMS_UNREACHABLE_DEFAULT_CASE() default: ::ams::impl::UnexpectedDefaultImpl(__PRETTY_FUNCTION__, __FILE__, __LINE__)
#else
#define AMS_CALL_ASSERT_FAIL_IMPL(type, expr, ...) ::ams::diag::OnAssertionFailure(type, "", "", "", 0)
#define AMS_CALL_ABORT_IMPL(expr, ...)       ::ams::diag::AbortImpl("", "", "", 0); AMS_UNUSED(expr, ## __VA_ARGS__)
#define AMS_UNREACHABLE_DEFAULT_CASE() default: ::ams::impl::UnexpectedDefaultImpl("", "", 0)
#endif

#ifdef AMS_ENABLE_ASSERTIONS
#define AMS_ASSERT_IMPL(type, expr, ...)                                                              \
    {                                                                                                 \
        if (std::is_constant_evaluated()) {                                                           \
            AMS_ASSUME(static_cast<bool>(expr));                                                      \
        } else {                                                                                      \
            if (const bool __tmp_ams_assert_val = static_cast<bool>(expr); (!__tmp_ams_assert_val)) { \
                AMS_CALL_ASSERT_FAIL_IMPL(type, #expr, ## __VA_ARGS__);                               \
            }                                                                                         \
        }                                                                                             \
    }
#elif defined(AMS_PRESERVE_ASSERTION_EXPRESSIONS)
#define AMS_ASSERT_IMPL(type, expr, ...) AMS_UNUSED(expr, ## __VA_ARGS__)
#else
#define AMS_ASSERT_IMPL(type, expr, ...) static_cast<void>(0)
#endif

#define AMS_ASSERT(expr, ...) AMS_ASSERT_IMPL(::ams::diag::AssertionType_Assert, expr, ## __VA_ARGS__)


#ifdef AMS_BUILD_FOR_AUDITING
#define AMS_AUDIT(expr, ...) AMS_ASSERT_IMPL(::ams::diag::AssertionType_Audit, expr, ## __VA_ARGS__)
#elif defined(AMS_PRESERVE_AUDIT_EXPRESSIONS)
#define AMS_AUDIT(expr, ...) AMS_UNUSED(expr, ## __VA_ARGS__)
#else
#define AMS_AUDIT(expr, ...) static_cast<void>(0)
#endif

#define AMS_ABORT(...) AMS_CALL_ABORT_IMPL("", ## __VA_ARGS__)

#define AMS_ABORT_UNLESS(expr, ...)                                                                               \
    {                                                                                                             \
        if (std::is_constant_evaluated()) {                                                                       \
            AMS_ASSUME(static_cast<bool>(expr));                                                                  \
        } else {                                                                                                  \
            if (const bool __tmp_ams_assert_val = static_cast<bool>(expr); AMS_UNLIKELY(!__tmp_ams_assert_val)) { \
                AMS_CALL_ABORT_IMPL(#expr, ##__VA_ARGS__);                                                        \
            }                                                                                                     \
        }                                                                                                         \
    }
