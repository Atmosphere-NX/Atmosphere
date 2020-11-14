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

namespace ams::util {

    template<typename T>
    constexpr T ToLower(T c) {
        return ('A' <= c && c <= 'Z') ? (c - 'A' + 'a') : c;
    }

    template<typename T>
    constexpr T ToUpper(T c) {
        return ('a' <= c && c <= 'z') ? (c - 'a' + 'A') : c;
    }

    template<typename T>
    int Strncmp(const T *lhs, const T *rhs, int count) {
        AMS_ASSERT(lhs != nullptr);
        AMS_ASSERT(rhs != nullptr);
        AMS_ABORT_UNLESS(count >= 0);

        if (count == 0) {
            return 0;
        }

        T l, r;
        do {
            l = *(lhs++);
            r = *(rhs++);
        } while (l && (l == r) && (--count));

        return l - r;
    }

    template<typename T>
    int Strnicmp(const T *lhs, const T *rhs, int count) {
        AMS_ASSERT(lhs != nullptr);
        AMS_ASSERT(rhs != nullptr);
        AMS_ABORT_UNLESS(count >= 0);

        if (count == 0) {
            return 0;
        }

        T l, r;
        do {
            l = ::ams::util::ToLower(*(lhs++));
            r = ::ams::util::ToLower(*(rhs++));
        } while (l && (l == r) && (--count));

        return l - r;
    }

    template<typename T>
    constexpr int Strlcpy(T *dst, const T *src, int count) {
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(src != nullptr);

        const T *cur = src;
        if (count > 0) {
            while ((--count) && *cur) {
                *(dst++) = *(cur++);
            }
            *dst = 0;
        }

        while (*cur) {
            cur++;
        }

        return static_cast<int>(cur - src);
    }

}
