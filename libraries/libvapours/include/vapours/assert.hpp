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

namespace ams::diag {

    NORETURN NOINLINE void AssertionFailureImpl(const char *file, int line, const char *func, const char *expr, u64 value, const char *format, ...) __attribute__((format(printf, 6, 7)));
    NORETURN NOINLINE void AssertionFailureImpl(const char *file, int line, const char *func, const char *expr, u64 value);

    NORETURN NOINLINE void AbortImpl(const char *file, int line, const char *func, const char *expr, u64 value, const char *format, ...) __attribute__((format(printf, 6, 7)));
    NORETURN NOINLINE void AbortImpl(const char *file, int line, const char *func, const char *expr, u64 value);
    NORETURN NOINLINE void AbortImpl();

}

#ifdef AMS_ENABLE_DETAILED_ASSERTIONS
#define AMS_CALL_ASSERT_FAIL_IMPL(cond, ...) ::ams::diag::AssertionFailureImpl(__FILE__, __LINE__, __PRETTY_FUNCTION__, cond, 0, ## __VA_ARGS__)
#define AMS_CALL_ABORT_IMPL(cond, ...)  ::ams::diag::AbortImpl(__FILE__, __LINE__, __PRETTY_FUNCTION__, cond, 0, ## __VA_ARGS__)
#else
#define AMS_CALL_ASSERT_FAIL_IMPL(cond, ...) ::ams::diag::AssertionFailureImpl("", 0, "", "", 0)
#define AMS_CALL_ABORT_IMPL(cond, ...)  ::ams::diag::AbortImpl(); AMS_UNUSED(cond, ## __VA_ARGS__)
#endif

#ifdef AMS_ENABLE_ASSERTIONS
#define AMS_ASSERT_IMPL(expr, ...)                                                                            \
    ({                                                                                                        \
        if (const bool __tmp_ams_assert_val = static_cast<bool>(expr); AMS_UNLIKELY(!__tmp_ams_assert_val)) { \
            AMS_CALL_ASSERT_FAIL_IMPL(#expr, ## __VA_ARGS__);                                                 \
        }                                                                                                     \
    })
#else
#define AMS_ASSERT_IMPL(expr, ...) AMS_UNUSED(expr, ## __VA_ARGS__)
#endif

#define AMS_ASSERT(expr, ...) AMS_ASSERT_IMPL(expr, ## __VA_ARGS__)

#define AMS_UNREACHABLE_DEFAULT_CASE() default: AMS_CALL_ABORT_IMPL("Unreachable default case entered")

#ifdef AMS_BUILD_FOR_AUDITING
#define AMS_AUDIT(expr, ...) AMS_ASSERT(expr, ## __VA_ARGS__)
#else
#define AMS_AUDIT(expr, ...) AMS_UNUSED(expr, ## __VA_ARGS__)
#endif

#define AMS_ABORT(...) AMS_CALL_ABORT_IMPL("", ## __VA_ARGS__)

#define AMS_ABORT_UNLESS(expr, ...)                                                                           \
    ({                                                                                                        \
        if (const bool __tmp_ams_assert_val = static_cast<bool>(expr); AMS_UNLIKELY(!__tmp_ams_assert_val)) { \
            AMS_CALL_ABORT_IMPL(#expr, ##__VA_ARGS__);                                                        \
        }                                                                                                     \
    })
