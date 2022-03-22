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
#include <stratosphere/sf/sf_common.hpp>
#include <stratosphere/sf/sf_out.hpp>

namespace ams::sf {

    class ISharedObject {
        NON_COPYABLE(ISharedObject);
        NON_MOVEABLE(ISharedObject);
        protected:
            constexpr ISharedObject() { /* ... */ }
            ~ISharedObject() { /* ... */ }
        public:
            constexpr virtual void AddReference() = 0;
            constexpr virtual void Release() = 0;
    };

    namespace impl {

        class SharedPointerBase {
            private:
                ISharedObject *m_ptr;
            private:
                constexpr void AddReferenceImpl() const {
                    if (m_ptr != nullptr) {
                        m_ptr->AddReference();
                    }
                }

                constexpr void ReleaseImpl() const {
                    if (m_ptr != nullptr) {
                        m_ptr->Release();
                    }
                }
            public:
                constexpr SharedPointerBase() : m_ptr(nullptr) { /* ... */ }

                constexpr SharedPointerBase(ISharedObject *ptr, bool incref) : m_ptr(ptr) {
                    if (incref) {
                        this->AddReferenceImpl();
                    }
                }

                constexpr ~SharedPointerBase() {
                    this->ReleaseImpl();
                }

                constexpr SharedPointerBase(const SharedPointerBase &rhs) : m_ptr(rhs.m_ptr) {
                    this->AddReferenceImpl();
                }

                constexpr SharedPointerBase(SharedPointerBase &&rhs) : m_ptr(rhs.m_ptr) {
                    rhs.m_ptr = nullptr;
                }

                constexpr SharedPointerBase &operator=(const SharedPointerBase &rhs) {
                    SharedPointerBase tmp(rhs);
                    tmp.swap(*this);
                    return *this;
                }

                constexpr SharedPointerBase &operator=(SharedPointerBase &&rhs) {
                    SharedPointerBase tmp(std::move(rhs));
                    tmp.swap(*this);
                    return *this;
                }

                constexpr void swap(SharedPointerBase &rhs) {
                    std::swap(m_ptr, rhs.m_ptr);
                }

                constexpr ISharedObject *Detach() {
                    ISharedObject *ret = m_ptr;
                    m_ptr = nullptr;
                    return ret;
                }

                constexpr ISharedObject *Get() const {
                    return m_ptr;
                }
        };

    }

    template<typename I>
    class SharedPointer {
        template<typename> friend class ::ams::sf::SharedPointer;
        template<typename> friend class ::ams::sf::Out;
        public:
            using Interface = I;
        private:
            impl::SharedPointerBase m_base;
        public:
            constexpr SharedPointer() : m_base() { /* ... */ }

            constexpr SharedPointer(std::nullptr_t) : m_base() { /* ... */ }

            constexpr SharedPointer(Interface *ptr, bool incref) : m_base(static_cast<ISharedObject *>(ptr), incref) { /* ... */ }

            constexpr SharedPointer(const SharedPointer &rhs) : m_base(rhs.m_base) { /* ... */ }

            constexpr SharedPointer(SharedPointer &&rhs) : m_base(std::move(rhs.m_base)) { /* ... */ }

            template<typename U> requires std::derived_from<U, Interface>
            constexpr SharedPointer(const SharedPointer<U> &rhs) : m_base(rhs.m_base) { /* ... */ }

            template<typename U> requires std::derived_from<U, Interface>
            constexpr SharedPointer(SharedPointer<U> &&rhs) : m_base(std::move(rhs.m_base)) { /* ... */ }

            constexpr SharedPointer &operator=(std::nullptr_t) {
                SharedPointer().swap(*this);
                return *this;
            }

            constexpr SharedPointer &operator=(const SharedPointer &rhs) {
                SharedPointer tmp(rhs);
                tmp.swap(*this);
                return *this;
            }

            constexpr SharedPointer &operator=(SharedPointer &&rhs) {
                SharedPointer tmp(std::move(rhs));
                tmp.swap(*this);
                return *this;
            }

            template<typename U> requires std::derived_from<U, Interface>
            constexpr SharedPointer &operator=(const SharedPointer<U> &rhs) {
                SharedPointer tmp(rhs);
                tmp.swap(*this);
                return *this;
            }

            template<typename U> requires std::derived_from<U, Interface>
            constexpr SharedPointer &operator=(SharedPointer<U> &&rhs) {
                SharedPointer tmp(std::move(rhs));
                tmp.swap(*this);
                return *this;
            }

            constexpr void swap(SharedPointer &rhs) {
                m_base.swap(rhs.m_base);
            }

            constexpr Interface *Detach() {
                return static_cast<Interface *>(m_base.Detach());
            }

            constexpr void Reset() {
                *this = nullptr;
            }

            constexpr Interface *Get() const {
                return static_cast<Interface *>(m_base.Get());
            }

            constexpr Interface *operator->() const {
                AMS_ASSERT(this->Get() != nullptr);
                return this->Get();
            }

            constexpr bool operator!() const {
                return this->Get() == nullptr;
            }

            constexpr bool operator==(std::nullptr_t) const {
                return this->Get() == nullptr;
            }

            constexpr bool operator!=(std::nullptr_t) const {
                return this->Get() != nullptr;
            }
    };

    template<typename Interface>
    constexpr void Swap(SharedPointer<Interface> &lhs, SharedPointer<Interface> &rhs) {
        lhs.swap(rhs);
    }

    constexpr inline void ReleaseSharedObject(ISharedObject *ptr) {
        ptr->Release();
    }

}
