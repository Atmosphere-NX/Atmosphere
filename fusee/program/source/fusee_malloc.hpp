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
#include <vapours.hpp>
#pragma once

namespace ams::nxboot {

    void *AllocateMemory(size_t size);

    ALWAYS_INLINE void *AllocateAligned(size_t size, size_t align) {
        return reinterpret_cast<void *>(util::AlignUp(reinterpret_cast<uintptr_t>(AllocateMemory(size + align)), align));
    }

    template<typename T, typename... Args> requires std::constructible_from<T, Args...>
    inline T *AllocateObject(Args &&... args) {
        T * const obj = static_cast<T *>(AllocateAligned(sizeof(T), alignof(T)));

        std::construct_at(obj, std::forward<Args>(args)...);

        return obj;
    }

}