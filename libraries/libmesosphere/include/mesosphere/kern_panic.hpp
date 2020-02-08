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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_debug_log.hpp>

namespace ams::kern {

    NORETURN void Panic(const char *file, int line, const char *format, ...) __attribute__((format(printf, 3, 4)));
    NORETURN void Panic();

}

#ifdef MESOSPHERE_ENABLE_DEBUG_PRINT
#define MESOSPHERE_PANIC(...) ::ams::kern::Panic(__FILE__, __LINE__, __VA_ARGS__)
#else
#define MESOSPHERE_PANIC(...) ::ams::kern::Panic()
#endif

#ifdef MESOSPHERE_ENABLE_ASSERTIONS
#define MESOSPHERE_ASSERT_IMPL(expr, ...)           \
    ({                                              \
        const bool __tmp_meso_assert_val = (expr);  \
        if (AMS_UNLIKELY(!__tmp_meso_assert_val)) { \
            MESOSPHERE_PANIC(__VA_ARGS__);          \
        }                                           \
    })
#else
#define MESOSPHERE_ASSERT_IMPL(expr, ...) do { static_cast<void>(expr); } while (0)
#endif

#define MESOSPHERE_ASSERT(expr)   MESOSPHERE_ASSERT_IMPL(expr, "Assertion failed: %s", #expr)
#define MESOSPHERE_R_ASSERT(expr) MESOSPHERE_ASSERT_IMPL(R_SUCCEEDED(expr), "Result assertion failed: %s", #expr)
#define MESOSPHERE_UNREACHABLE_DEFAULT_CASE() default: MESOSPHERE_PANIC("Unreachable default case entered")

#ifdef MESOSPHERE_ENABLE_THIS_ASSERT
#define MESOSPHERE_ASSERT_THIS()  MESOSPHERE_ASSERT(this != nullptr)
#else
#define MESOSPHERE_ASSERT_THIS()
#endif

#define MESOSPHERE_TODO(arg) ({ constexpr const char *__mesosphere_todo = arg; MESOSPHERE_PANIC("TODO (%s): %s", __PRETTY_FUNCTION__, __mesosphere_todo); })
#define MESOSPHERE_TODO_IMPLEMENT() MESOSPHERE_TODO("Implement")

#define MESOSPHERE_ABORT() MESOSPHERE_PANIC("Abort()");
#define MESOSPHERE_INIT_ABORT() do { /* ... */ } while (true)

#define MESOSPHERE_ABORT_UNLESS(expr)               \
    ({                                              \
        const bool _tmp_meso_assert_val = (expr);   \
        if (AMS_UNLIKELY(!_tmp_meso_assert_val)) {  \
            MESOSPHERE_PANIC("Abort(): %s", #expr); \
        }                                           \
    })

#define MESOSPHERE_INIT_ABORT_UNLESS(expr)          \
    ({                                              \
        const bool __tmp_meso_assert_val = (expr);  \
        if (AMS_UNLIKELY(!__tmp_meso_assert_val)) { \
            MESOSPHERE_INIT_ABORT();                \
        }                                           \
    })

#define MESOSPHERE_R_ABORT_UNLESS(expr)                                                                                                          \
    ({                                                                                                                                           \
        const ::ams::Result _tmp_meso_r_abort_res = static_cast<::ams::Result>((expr));                                                          \
        if (AMS_UNLIKELY((R_FAILED(_tmp_meso_r_abort_res)))) {                                                                                   \
            MESOSPHERE_PANIC("Result Abort(): %s 2%03d-%04d", #expr, _tmp_meso_r_abort_res.GetModule(), _tmp_meso_r_abort_res.GetDescription()); \
        }                                                                                                                                        \
    })
