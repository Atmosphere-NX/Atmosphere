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

    template<typename T, size_t Size = sizeof(T), size_t Align = alignof(T)>
    struct TypedStorage {
        typename std::aligned_storage<Size, Align>::type _storage;
    };

    template<typename T>
    static ALWAYS_INLINE T *GetPointer(TypedStorage<T> &ts) {
        return std::launder(reinterpret_cast<T *>(std::addressof(ts._storage)));
    }

    template<typename T>
    static ALWAYS_INLINE const T *GetPointer(const TypedStorage<T> &ts) {
        return std::launder(reinterpret_cast<const T *>(std::addressof(ts._storage)));
    }

    template<typename T>
    static ALWAYS_INLINE T &GetReference(TypedStorage<T> &ts) {
        return *GetPointer(ts);
    }

    template<typename T>
    static ALWAYS_INLINE const T &GetReference(const TypedStorage<T> &ts) {
        return *GetPointer(ts);
    }

    namespace impl {

        template<typename T>
        static ALWAYS_INLINE T *GetPointerForConstructAt(TypedStorage<T> &ts) {
            return reinterpret_cast<T *>(std::addressof(ts._storage));
        }

    }

    template<typename T, typename... Args>
    static ALWAYS_INLINE T *ConstructAt(TypedStorage<T> &ts, Args &&... args) {
        return std::construct_at(impl::GetPointerForConstructAt(ts), std::forward<Args>(args)...);
    }

    template<typename T>
    static ALWAYS_INLINE void DestroyAt(TypedStorage<T> &ts) {
        return std::destroy_at(GetPointer(ts));
    }

    namespace impl {

        template<typename T>
        class TypedStorageGuard {
            NON_COPYABLE(TypedStorageGuard);
            private:
                TypedStorage<T> &m_ts;
                bool m_active;
            public:
                template<typename... Args>
                ALWAYS_INLINE TypedStorageGuard(TypedStorage<T> &ts, Args &&... args) : m_ts(ts), m_active(true) {
                    ConstructAt(m_ts, std::forward<Args>(args)...);
                }

                ALWAYS_INLINE ~TypedStorageGuard() { if (m_active) { DestroyAt(m_ts); } }

                ALWAYS_INLINE void Cancel() { m_active = false; }

                ALWAYS_INLINE TypedStorageGuard(TypedStorageGuard&& rhs) : m_ts(rhs.m_ts), m_active(rhs.m_active) {
                    rhs.Cancel();
                }

                TypedStorageGuard &operator=(TypedStorageGuard&& rhs) = delete;
        };

    }

    template<typename T, typename... Args>
    static ALWAYS_INLINE impl::TypedStorageGuard<T> ConstructAtGuarded(TypedStorage<T> &ts, Args &&... args) {
        return impl::TypedStorageGuard<T>(ts, std::forward<Args>(args)...);
    }

}
