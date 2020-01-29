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
#include <vapours.hpp>

namespace ams::kern {

    NORETURN void Panic(const char *file, int line, const char *format, ...);
    NORETURN void Panic();

}

#ifdef MESOSPHERE_ENABLE_DEBUG_PRINT
#define MESOSPHERE_PANIC(...) ams::kern::Panic(__FILE__, __LINE__, __VA_ARGS__)
#else
#define MESOSPHERE_PANIC(...) ams::kern::Panic()
#endif

#ifdef MESOSPHERE_ENABLE_ASSERTIONS
#define MESOSPHERE_ASSERT_IMPL(expr, ...)  \
    ({                                     \
        if (AMS_UNLIKELY(!(expr))) {         \
            MESOSPHERE_PANIC(__VA_ARGS__); \
        }                                  \
    })
#else
#define MESOSPHERE_ASSERT_IMPL(expr, ...) do { } while (0)
#endif

#define MESOSPHERE_ASSERT(expr)   MESOSPHERE_ASSERT_IMPL(expr, "Assertion failed: %s", #expr)
#define MESOSPHERE_R_ASSERT(expr) MESOSPHERE_ASSERT_IMPL(R_SUCCEEDED(expr), "Result assertion failed: %s", #expr)

#define MESOSPHERE_ABORT() MESOSPHERE_PANIC("Abort()");
#define MESOSPHERE_INIT_ABORT() do { /* ... */ } while (true)

#define MESOSPHERE_ABORT_UNLESS(expr)               \
    ({                                              \
        if (AMS_UNLIKELY(!(expr))) {                \
            MESOSPHERE_PANIC("Abort(): %s", #expr); \
        }                                           \
    })

#define MESOSPHERE_INIT_ABORT_UNLESS(expr)          \
    ({                                              \
        if (AMS_UNLIKELY(!(expr))) {                \
            MESOSPHERE_INIT_ABORT();                \
        }                                           \
    })
