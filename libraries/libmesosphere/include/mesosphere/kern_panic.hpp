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

    NORETURN NOINLINE void Panic(const char *file, int line, const char *format, ...) __attribute__((format(printf, 3, 4)));
    NORETURN NOINLINE void Panic();

}

#define MESOSPHERE_UNUSED(...) AMS_UNUSED(__VA_ARGS__)

#ifdef MESOSPHERE_ENABLE_DEBUG_PRINT
#define MESOSPHERE_PANIC(...) do { ::ams::kern::Panic(__FILE__, __LINE__, ## __VA_ARGS__); } while(0)
#else
#define MESOSPHERE_PANIC(...) do { MESOSPHERE_UNUSED(__VA_ARGS__); ::ams::kern::Panic(); } while(0)
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

#define MESOSPHERE_ASSERT(expr)   MESOSPHERE_ASSERT_IMPL(expr, "Assertion failed: %s\n", #expr)
#define MESOSPHERE_R_ASSERT(expr) MESOSPHERE_ASSERT_IMPL(R_SUCCEEDED(expr), "Result assertion failed: %s\n", #expr)
#define MESOSPHERE_UNREACHABLE_DEFAULT_CASE() default: MESOSPHERE_PANIC("Unreachable default case entered")

#ifdef MESOSPHERE_ENABLE_THIS_ASSERT
#define MESOSPHERE_ASSERT_THIS()  MESOSPHERE_ASSERT(this != nullptr)
#else
#define MESOSPHERE_ASSERT_THIS()
#endif

#ifdef MESOSPHERE_BUILD_FOR_AUDITING
#define MESOSPHERE_AUDIT(expr) MESOSPHERE_ASSERT(expr)
#else
#define MESOSPHERE_AUDIT(expr) do { static_cast<void>(expr); } while (0)
#endif

#define MESOSPHERE_TODO(arg) ({ constexpr const char *__mesosphere_todo = arg; static_cast<void>(__mesosphere_todo); MESOSPHERE_PANIC("TODO (%s): %s\n", __PRETTY_FUNCTION__, __mesosphere_todo); })
#define MESOSPHERE_UNIMPLEMENTED() MESOSPHERE_PANIC("%s: Unimplemented\n", __PRETTY_FUNCTION__)

#define MESOSPHERE_ABORT() MESOSPHERE_PANIC("Abort()\n");
#define MESOSPHERE_INIT_ABORT() AMS_INFINITE_LOOP()

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

#define MESOSPHERE_R_ABORT_UNLESS(expr)                                                                                                            \
    ({                                                                                                                                             \
        const ::ams::Result _tmp_meso_r_abort_res = static_cast<::ams::Result>((expr));                                                            \
        if (AMS_UNLIKELY((R_FAILED(_tmp_meso_r_abort_res)))) {                                                                                     \
            MESOSPHERE_PANIC("Result Abort(): %s 2%03d-%04d\n", #expr, _tmp_meso_r_abort_res.GetModule(), _tmp_meso_r_abort_res.GetDescription()); \
        }                                                                                                                                          \
    })
