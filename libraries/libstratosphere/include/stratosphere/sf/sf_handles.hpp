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
#include "sf_common.hpp"
#include "sf_out.hpp"
#include "cmif/sf_cmif_pointer_and_size.hpp"

namespace ams::sf {

    namespace impl {

        struct InHandleTag{};
        struct OutHandleTag{};

        template<u32 Attribute>
        struct InHandle : public InHandleTag {
            ::Handle handle;

            constexpr InHandle() : handle(INVALID_HANDLE) { /* ... */ }
            constexpr InHandle(::Handle h) : handle(h) { /* ... */ }
            constexpr InHandle(const InHandle &o) : handle(o.handle) { /* ... */ }

            constexpr void operator=(const ::Handle &h) { this->handle = h; }
            constexpr void operator=(const InHandle &o) { this->handle = o.handle; }

            constexpr /* TODO: explicit? */ operator ::Handle() const { return this->handle; }
            constexpr ::Handle GetValue() const { return this->handle; }
        };

        template<typename T>
        class OutHandleImpl : public OutHandleTag {
            static_assert(std::is_base_of<InHandleTag, T>::value, "OutHandleImpl requires InHandle base");
            private:
                T *ptr;
            public:
                constexpr OutHandleImpl(T *p) : ptr(p) { /* ... */ }

                constexpr void SetValue(const Handle &value) {
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

                constexpr Handle *GetHandlePointer() const {
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

    static_assert(sizeof(MoveHandle) == sizeof(::Handle), "sizeof(MoveHandle)");
    static_assert(sizeof(CopyHandle) == sizeof(::Handle), "sizeof(CopyHandle)");

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

            constexpr void SetValue(const Handle &value) {
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

            constexpr Handle *GetHandlePointer() const {
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

            constexpr void SetValue(const Handle &value) {
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

            constexpr Handle *GetHandlePointer() const {
                return Base::GetHandlePointer();
            }

            constexpr T &operator *() const {
                return Base::operator*();
            }

            constexpr T *operator ->() const {
                return Base::operator->();
            }
    };

    using OutMoveHandle = sf::Out<sf::MoveHandle>;
    using OutCopyHandle = sf::Out<sf::CopyHandle>;

}
