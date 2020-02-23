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

    template<typename T, size_t Size, size_t Align>
    struct TypedStorage {
        typename std::aligned_storage<Size, Align>::type _storage;
    };

    #define TYPED_STORAGE(...) ::ams::util::TypedStorage<__VA_ARGS__, sizeof(__VA_ARGS__), alignof(__VA_ARGS__)>

    template<typename T>
    static constexpr ALWAYS_INLINE T *GetPointer(TYPED_STORAGE(T) &ts) {
        return static_cast<T *>(static_cast<void *>(std::addressof(ts._storage)));
    }

    template<typename T>
    static constexpr ALWAYS_INLINE const T *GetPointer(const TYPED_STORAGE(T) &ts) {
        return static_cast<const T *>(static_cast<const void *>(std::addressof(ts._storage)));
    }

    template<typename T>
    static constexpr ALWAYS_INLINE T &GetReference(TYPED_STORAGE(T) &ts) {
        return *GetPointer(ts);
    }

    template<typename T>
    static constexpr ALWAYS_INLINE const T &GetReference(const TYPED_STORAGE(T) &ts) {
        return *GetPointer(ts);
    }

}
