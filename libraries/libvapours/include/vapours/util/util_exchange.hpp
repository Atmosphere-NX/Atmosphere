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

    template<typename T, typename U> requires (std::is_assignable<T&, U&&>::value && std::is_copy_constructible<T>::value && std::is_move_constructible<T>::value)
    constexpr inline T Exchange(T *ptr, U value) {
        AMS_ASSERT(ptr != nullptr);
        auto ret = std::move(*ptr);
        *ptr = std::move(value);
        return ret;
    }

    template<typename T>
    constexpr inline T *Exchange(T **ptr, std::nullptr_t) {
        AMS_ASSERT(ptr != nullptr);
        auto ret(*ptr);
        *ptr = nullptr;
        return ret;
    }

}
