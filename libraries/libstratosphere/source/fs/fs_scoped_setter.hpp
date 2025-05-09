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
#include <stratosphere.hpp>

namespace ams::fs {

    template<typename T>
    class ScopedSetter {
        NON_COPYABLE(ScopedSetter);
        private:
            T *m_ptr;
            T m_value;
        public:
            constexpr ALWAYS_INLINE ScopedSetter(T &p, T v) : m_ptr(std::addressof(p)), m_value(v) { /* ... */ }
            ALWAYS_INLINE ~ScopedSetter() {
                if (m_ptr) {
                    *m_ptr = m_value;
                }
            }

            ALWAYS_INLINE ScopedSetter(ScopedSetter &&rhs) {
                m_ptr = rhs.m_ptr;
                m_value = rhs.m_value;
                rhs.Reset();
            }

            ALWAYS_INLINE ScopedSetter &operator=(ScopedSetter &&rhs) {
                m_ptr = rhs.m_ptr;
                m_value = rhs.m_value;
                rhs.Reset();
                return *this;
            }

            ALWAYS_INLINE void Set(T v) { m_value = v; }
        private:
            ALWAYS_INLINE void Reset() {
                m_ptr = nullptr;
            }
    };

    template<typename T>
    ALWAYS_INLINE ScopedSetter<T> MakeScopedSetter(T &p, T v) {
        return ScopedSetter<T>(p, v);
    }

}
