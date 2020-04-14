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
