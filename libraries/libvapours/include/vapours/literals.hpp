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

namespace ams { inline namespace literals {

    constexpr ALWAYS_INLINE u64 operator ""_KB(unsigned long long n) {
        return static_cast<u64>(n) * UINT64_C(1024);
    }

    constexpr ALWAYS_INLINE u64 operator ""_MB(unsigned long long n) {
        return operator ""_KB(n) * UINT64_C(1024);
    }

    constexpr ALWAYS_INLINE u64 operator ""_GB(unsigned long long n) {
        return operator ""_MB(n) * UINT64_C(1024);
    }

} }
