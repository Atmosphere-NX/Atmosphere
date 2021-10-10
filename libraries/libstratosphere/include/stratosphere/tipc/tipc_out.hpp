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
#include <stratosphere/tipc/tipc_common.hpp>
#include <stratosphere/tipc/tipc_pointer_and_size.hpp>

namespace ams::tipc {

    namespace impl {

        struct OutBaseTag{};

    }

    template<typename>
    struct IsOutForceEnabled : public std::false_type{};

    template<>
    struct IsOutForceEnabled<::ams::Result> : public std::true_type{};

    template<typename T>
    concept OutEnabled = (std::is_trivial<T>::value || IsOutForceEnabled<T>::value) && !std::is_pointer<T>::value;

    template<typename T>
    class Out : public impl::OutBaseTag {
        static_assert(OutEnabled<T>);
        public:
            static constexpr size_t TypeSize = sizeof(T);
        private:
            T *m_ptr;
        public:
            constexpr Out(uintptr_t p) : m_ptr(reinterpret_cast<T *>(p)) { /* ... */ }
            constexpr Out(T *p) : m_ptr(p) { /* ... */ }
            constexpr Out(const tipc::PointerAndSize &pas) : m_ptr(reinterpret_cast<T *>(pas.GetAddress())) { /* TODO: Is AMS_ABORT_UNLESS(pas.GetSize() >= sizeof(T)); necessary? */ }

            ALWAYS_INLINE void SetValue(const T& value) const {
                *m_ptr = value;
            }

            ALWAYS_INLINE const T &GetValue() const {
                return *m_ptr;
            }

            ALWAYS_INLINE T *GetPointer() const {
                return m_ptr;
            }

            /* Convenience operators. */
            ALWAYS_INLINE T &operator*() const {
                return *m_ptr;
            }

            ALWAYS_INLINE T *operator->() const {
                return m_ptr;
            }
    };

    template<typename T>
    class Out<T *> {
        static_assert(!std::is_same<T, T>::value, "Invalid tipc::Out<T> (Raw Pointer)");
    };

}
