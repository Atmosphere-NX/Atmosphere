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
#include <stratosphere.hpp>

namespace ams::fs {

    template<typename T>
    class ScopedSetter {
        NON_COPYABLE(ScopedSetter);
        private:
            T *ptr;
            T value;
        public:
            constexpr ALWAYS_INLINE ScopedSetter(T &p, T v) : ptr(std::addressof(p)), value(v) { /* ... */ }
            ALWAYS_INLINE ~ScopedSetter() {
                if (this->ptr) {
                    *this->ptr = this->value;
                }
            }

            ALWAYS_INLINE ScopedSetter(ScopedSetter &&rhs) {
                this->ptr = rhs.ptr;
                this->value = rhs.value;
                rhs.Reset();
            }

            ALWAYS_INLINE ScopedSetter &operator=(ScopedSetter &&rhs) {
                this->ptr = rhs.ptr;
                this->value = rhs.value;
                rhs.Reset();
                return *this;
            }

            ALWAYS_INLINE void Set(T v) { this->value = v; }
        private:
            ALWAYS_INLINE void Reset() {
                this->ptr = nullptr;
            }
    };

    template<typename T>
    ALWAYS_INLINE ScopedSetter<T> MakeScopedSetter(T &p, T v) {
        return ScopedSetter<T>(p, v);
    }

}
