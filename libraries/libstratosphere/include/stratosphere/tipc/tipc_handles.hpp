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
#include <stratosphere/tipc/tipc_out.hpp>
#include <stratosphere/tipc/tipc_pointer_and_size.hpp>

namespace ams::tipc {

    /* TODO: How do InHandles work in tipc? No examples to work off of. */
    class CopyHandle {
        private:
            CopyHandle();
    };

    class MoveHandle {
        private:
            MoveHandle();
    };

    template<>
    class Out<CopyHandle> {
        private:
            os::NativeHandle * const m_ptr;
        public:
            ALWAYS_INLINE Out(os::NativeHandle *p) : m_ptr(p) { /* ... */ }

            ALWAYS_INLINE void SetValue(os::NativeHandle v) const {
                *m_ptr = v;
            }

            ALWAYS_INLINE const os::NativeHandle &GetValue() const {
                return *m_ptr;
            }

            ALWAYS_INLINE os::NativeHandle *GetPointer() const {
                return m_ptr;
            }

            /* Convenience operators. */
            ALWAYS_INLINE os::NativeHandle &operator*() const {
                return *m_ptr;
            }

            ALWAYS_INLINE os::NativeHandle *operator->() const {
                return m_ptr;
            }
    };

    template<>
    class Out<MoveHandle> {
        private:
            os::NativeHandle * const m_ptr;
        public:
            ALWAYS_INLINE Out(os::NativeHandle *p) : m_ptr(p) { /* ... */ }

            ALWAYS_INLINE void SetValue(os::NativeHandle v) const {
                *m_ptr = v;
            }

            ALWAYS_INLINE const os::NativeHandle &GetValue() const {
                return *m_ptr;
            }

            ALWAYS_INLINE os::NativeHandle *GetPointer() const {
                return m_ptr;
            }

            /* Convenience operators. */
            ALWAYS_INLINE os::NativeHandle &operator*() const {
                return *m_ptr;
            }

            ALWAYS_INLINE os::NativeHandle *operator->() const {
                return m_ptr;
            }
    };

    using OutMoveHandle = tipc::Out<tipc::MoveHandle>;
    using OutCopyHandle = tipc::Out<tipc::CopyHandle>;

}
