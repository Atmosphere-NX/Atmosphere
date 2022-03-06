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
    constexpr int Strncmp(const T *lhs, const T *rhs, int count) {
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
    constexpr int Strnicmp(const T *lhs, const T *rhs, int count) {
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

    template<typename T>
    constexpr int Strlen(const T *str) {
        AMS_ASSERT(str != nullptr);

        int length = 0;
        while (*str++) {
            ++length;
        }

        return length;
    }

    template<typename T>
    constexpr int Strnlen(const T *str, int count) {
        AMS_ASSERT(str != nullptr);
        AMS_ASSERT(count >= 0);

        int length = 0;
        while (count-- && *str++) {
            ++length;
        }

        return length;
    }

    #if defined(ATMOSPHERE_IS_STRATOSPHERE)
    ALWAYS_INLINE void *Memchr(void *s, int c, size_t n) {
        return const_cast<void*>(::memchr(s, c, n));
    }

    ALWAYS_INLINE const void *Memchr(const void *s, int c, size_t n) {
        return ::memchr(s, c, n);
    }

    inline void *Memrchr(void *s, int c, size_t n) {
        #if !(defined(__MINGW32__) || defined(__MINGW64__) || defined(ATMOSPHERE_OS_MACOS))
        return const_cast<void *>(::memrchr(s, c, n));
        #else
        /* TODO: Optimized implementation? */
        if (AMS_LIKELY(n > 0)) {
            const u8 *p = static_cast<const u8 *>(s);
            const u8 v = static_cast<u8>(c);
            while ((n--) != 0) {
                if (p[n] == v) {
                    return const_cast<void *>(static_cast<const void *>(p + n));
                }
            }
        }
        return nullptr;
        #endif
    }

    inline const void *Memrchr(const void *s, int c, size_t n) {
        #if !(defined(__MINGW32__) || defined(__MINGW64__) || defined(ATMOSPHERE_OS_MACOS))
        return ::memrchr(s, c, n);
        #else
        /* TODO: Optimized implementation? */
        if (AMS_LIKELY(n > 0)) {
            const u8 *p = static_cast<const u8 *>(s);
            const u8 v = static_cast<u8>(c);
            while ((n--) != 0) {
                if (p[n] == v) {
                    return static_cast<const void *>(p + n);
                }
            }
        }
        return nullptr;
        #endif
    }
    #endif

}
