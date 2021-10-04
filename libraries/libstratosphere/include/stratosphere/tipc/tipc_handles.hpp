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

    namespace impl {

        struct InHandleTag{};
        struct OutHandleTag{};

        template<u32 Attribute>
        struct InHandle : public InHandleTag {
            os::NativeHandle handle;

            constexpr InHandle() : handle(os::InvalidNativeHandle) { /* ... */ }
            constexpr InHandle(os::NativeHandle h) : handle(h) { /* ... */ }
            constexpr InHandle(const InHandle &o) : handle(o.handle) { /* ... */ }

            constexpr void operator=(const os::NativeHandle &h) { this->handle = h; }
            constexpr void operator=(const InHandle &o) { this->handle = o.handle; }

            constexpr /* TODO: explicit? */ operator os::NativeHandle() const { return this->handle; }
            constexpr os::NativeHandle GetValue() const { return this->handle; }
        };

        template<typename T>
        class OutHandleImpl : public OutHandleTag {
            static_assert(std::is_base_of<InHandleTag, T>::value, "OutHandleImpl requires InHandle base");
            private:
                T *ptr;
            public:
                constexpr OutHandleImpl(T *p) : ptr(p) { /* ... */ }

                constexpr void SetValue(const os::NativeHandle &value) {
                    *this->ptr = value;
                }

                constexpr void SetValue(const T &value) {
                    *this->ptr = value;
                }

                constexpr const T &GetValue() const {
                    return *this->ptr;
                }

                constexpr T *GetPointer() const {
                    return this->ptr;
                }

                constexpr os::NativeHandle *GetHandlePointer() const {
                    return &this->ptr->handle;
                }

                constexpr T &operator *() const {
                    return *this->ptr;
                }

                constexpr T *operator ->() const {
                    return this->ptr;
                }
        };

    }

    using MoveHandle = typename impl::InHandle<SfOutHandleAttr_HipcMove>;
    using CopyHandle = typename impl::InHandle<SfOutHandleAttr_HipcCopy>;

    static_assert(sizeof(MoveHandle) == sizeof(os::NativeHandle), "sizeof(MoveHandle)");
    static_assert(sizeof(CopyHandle) == sizeof(os::NativeHandle), "sizeof(CopyHandle)");

    template<>
    class IsOutForceEnabled<MoveHandle> : public std::true_type{};
    template<>
    class IsOutForceEnabled<CopyHandle> : public std::true_type{};

    template<>
    class Out<MoveHandle> : public impl::OutHandleImpl<MoveHandle> {
        private:
            using T = MoveHandle;
            using Base = impl::OutHandleImpl<T>;
        public:
            constexpr Out<T>(T *p) : Base(p) { /* ... */ }

            constexpr void SetValue(const os::NativeHandle &value) {
                Base::SetValue(value);
            }

            constexpr void SetValue(const T &value) {
                Base::SetValue(value);
            }

            constexpr const T &GetValue() const {
                return Base::GetValue();
            }

            constexpr T *GetPointer() const {
                return Base::GetPointer();
            }

            constexpr os::NativeHandle *GetHandlePointer() const {
                return Base::GetHandlePointer();
            }

            constexpr T &operator *() const {
                return Base::operator*();
            }

            constexpr T *operator ->() const {
                return Base::operator->();
            }
    };

    template<>
    class Out<CopyHandle> : public impl::OutHandleImpl<CopyHandle> {
        private:
            using T = CopyHandle;
            using Base = impl::OutHandleImpl<T>;
        public:
            constexpr Out<T>(T *p) : Base(p) { /* ... */ }

            constexpr void SetValue(const os::NativeHandle &value) {
                Base::SetValue(value);
            }

            constexpr void SetValue(const T &value) {
                Base::SetValue(value);
            }

            constexpr const T &GetValue() const {
                return Base::GetValue();
            }

            constexpr T *GetPointer() const {
                return Base::GetPointer();
            }

            constexpr os::NativeHandle *GetHandlePointer() const {
                return Base::GetHandlePointer();
            }

            constexpr T &operator *() const {
                return Base::operator*();
            }

            constexpr T *operator ->() const {
                return Base::operator->();
            }
    };

    using OutMoveHandle = tipc::Out<tipc::MoveHandle>;
    using OutCopyHandle = tipc::Out<tipc::CopyHandle>;

}
