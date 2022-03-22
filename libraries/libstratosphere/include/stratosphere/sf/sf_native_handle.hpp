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
#include <stratosphere/os/os_native_handle.hpp>
#include <stratosphere/sf/sf_common.hpp>
#include <stratosphere/sf/sf_out.hpp>

namespace ams::sf {

    class NativeHandle {
        protected:
            NON_COPYABLE(NativeHandle);
        private:
            os::NativeHandle m_handle;
            bool m_managed;
        public:
            constexpr NativeHandle() : m_handle(os::InvalidNativeHandle), m_managed(false) { /* ... */ }

            constexpr NativeHandle(os::NativeHandle handle, bool managed) : m_handle(handle), m_managed(managed) { /* ... */ }

            constexpr NativeHandle(NativeHandle &&rhs) : m_handle(rhs.m_handle), m_managed(rhs.m_managed) {
                rhs.m_managed = false;
                rhs.m_handle  = os::InvalidNativeHandle;
            }

            constexpr NativeHandle &operator=(NativeHandle &&rhs) {
                NativeHandle(std::move(rhs)).swap(*this);
                return *this;
            }

            constexpr ~NativeHandle() {
                if (m_managed) {
                    os::CloseNativeHandle(m_handle);
                }
            }

            constexpr void Detach() {
                m_managed = false;
                m_handle  = os::InvalidNativeHandle;
            }

            constexpr void Swap(NativeHandle &rhs) {
                std::swap(m_handle, rhs.m_handle);
                std::swap(m_managed, rhs.m_managed);
            }

            constexpr ALWAYS_INLINE void swap(NativeHandle &rhs) {
                return Swap(rhs);
            }

            constexpr NativeHandle GetShared() const {
                return NativeHandle(m_handle, false);
            }

            constexpr os::NativeHandle GetOsHandle() const {
                return m_handle;
            }

            constexpr bool IsManaged() const {
                return m_managed;
            }

            constexpr void Reset() {
                NativeHandle().swap(*this);
            }
    };

    class CopyHandle : public NativeHandle {
        public:
            using NativeHandle::NativeHandle;
            using NativeHandle::operator=;
    };

    class MoveHandle : public NativeHandle {
        public:
            using NativeHandle::NativeHandle;
            using NativeHandle::operator=;
    };

    constexpr ALWAYS_INLINE void swap(NativeHandle &lhs, NativeHandle &rhs) {
        lhs.swap(rhs);
    }

    template<>
    class Out<CopyHandle> {
        private:
            NativeHandle *m_ptr;
        public:
            Out(NativeHandle *p) : m_ptr(p) { /* ... */ }

            void SetValue(NativeHandle v) const {
                *m_ptr = std::move(v);
            }

            ALWAYS_INLINE void SetValue(os::NativeHandle os_handle, bool managed) const {
                return this->SetValue(NativeHandle(os_handle, managed));
            }

            NativeHandle &operator*() const {
                return *m_ptr;
            }
    };

    template<>
    class Out<MoveHandle> {
        private:
            NativeHandle *m_ptr;
        public:
            Out(NativeHandle *p) : m_ptr(p) { /* ... */ }

            void SetValue(NativeHandle v) const {
                *m_ptr = std::move(v);
            }

            ALWAYS_INLINE void SetValue(os::NativeHandle os_handle, bool managed) const {
                return this->SetValue(NativeHandle(os_handle, managed));
            }

            NativeHandle &operator*() const {
                return *m_ptr;
            }
    };

    using OutCopyHandle = Out<CopyHandle>;
    using OutMoveHandle = Out<MoveHandle>;

}