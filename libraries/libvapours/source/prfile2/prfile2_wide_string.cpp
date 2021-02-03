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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif

namespace ams::prfile2 {

    size_t w_strlen(const WideChar *s) {
        const WideChar *cur;
        for (cur = s; *cur != 0; ++cur) {
            /* ... */
        }
        return cur - s;
    }

    size_t w_strnlen(const WideChar *s, size_t length) {
        const WideChar *cur;
        for (cur = s; *cur != 0 && length != 0; ++cur, --length) {
            /* ... */
        }
        return cur - s;
    }

    WideChar *w_strcpy(WideChar *dst, const WideChar *src) {
        WideChar * const ret = dst;
        while (true) {
            const auto c = *(src++);
            *(dst++) = c;

            if (c == 0) {
                break;
            }
        }
        return ret;
    }

    WideChar *w_strncpy(WideChar *dst, const WideChar *src, size_t length) {
        WideChar * const ret = dst;
        while (length > 1) {
            const auto c = *(src++);
            *(dst++) = c;

            if (c == 0) {
                return ret;
            }
        }

        if (length == 1) {
            *(dst++) = 0;
        }

        return ret;
    }

    int w_strcmp(const WideChar *lhs, const WideChar *rhs) {
        WideChar l, r;
        while (true) {
            l = *(lhs++);
            r = *(rhs++);
            if (l == 0 || r == 0 || l != r) {
                break;
            }
        }

        return l - r;
    }

    int w_strncmp(const WideChar *lhs, const WideChar *rhs, size_t length) {
        if (length == 0) {
            return 0;
        }

        WideChar l, r;
        while (true) {
            l = *(lhs++);
            r = *(rhs++);
            if (l == 0 || r == 0 || l != r) {
                break;
            }

            if ((--length) == 0) {
                return 0;
            }
        }

        return l - r;
    }

}
