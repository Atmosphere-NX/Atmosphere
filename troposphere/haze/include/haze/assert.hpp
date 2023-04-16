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

#define HAZE_ASSERT(expr)                                       \
{                                                               \
    const bool __tmp_haze_assert_val = static_cast<bool>(expr); \
    if (AMS_UNLIKELY(!__tmp_haze_assert_val)) {                 \
        svcBreak(BreakReason_Assert, 0, 0);                     \
    }                                                           \
}

#define HAZE_R_ABORT_UNLESS(res_expr)          \
{                                              \
    const auto _tmp_r_abort_rc = (res_expr);   \
    HAZE_ASSERT(R_SUCCEEDED(_tmp_r_abort_rc)); \
}

#define HAZE_UNREACHABLE_DEFAULT_CASE() default: HAZE_ASSERT(false)
