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

        /* Buffer utilities. */
        struct BufferBaseTag{};

    }

    namespace impl {

        class BufferBase : public BufferBaseTag {
            public:
                static constexpr u32 AdditionalAttributes = 0;
            private:
                const tipc::PointerAndSize m_pas;
            protected:
                constexpr ALWAYS_INLINE uintptr_t GetAddressImpl() const {
                    return m_pas.GetAddress();
                }

                template<typename Entry>
                constexpr ALWAYS_INLINE size_t GetSizeImpl() const {
                    return m_pas.GetSize() / sizeof(Entry);
                }
            public:
                constexpr ALWAYS_INLINE BufferBase() : m_pas() { /* ... */ }
                constexpr ALWAYS_INLINE BufferBase(const tipc::PointerAndSize &pas) : m_pas(pas) { /* ... */ }
                constexpr ALWAYS_INLINE BufferBase(uintptr_t ptr, size_t sz) : m_pas(ptr, sz) { /* ... */ }
        };

        class InBufferBase : public BufferBase {
            public:
                using BaseType = BufferBase;
                static constexpr u32 AdditionalAttributes = BaseType::AdditionalAttributes |
                                                            SfBufferAttr_In;
            public:
                constexpr ALWAYS_INLINE InBufferBase() : BaseType() { /* ... */ }
                constexpr ALWAYS_INLINE InBufferBase(const tipc::PointerAndSize &pas) : BaseType(pas) { /* ... */ }
                constexpr ALWAYS_INLINE InBufferBase(uintptr_t ptr, size_t sz) : BaseType(ptr, sz) { /* ... */ }

                constexpr ALWAYS_INLINE InBufferBase(const void *ptr, size_t sz) : BaseType(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }
                constexpr ALWAYS_INLINE InBufferBase(const u8 *ptr, size_t sz) : BaseType(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }
        };

        class OutBufferBase : public BufferBase {
            public:
                using BaseType = BufferBase;
                static constexpr u32 AdditionalAttributes = BaseType::AdditionalAttributes |
                                                            SfBufferAttr_Out;
            public:
                constexpr ALWAYS_INLINE OutBufferBase() : BaseType() { /* ... */ }
                constexpr ALWAYS_INLINE OutBufferBase(const tipc::PointerAndSize &pas) : BaseType(pas) { /* ... */ }
                constexpr ALWAYS_INLINE OutBufferBase(uintptr_t ptr, size_t sz) : BaseType(ptr, sz) { /* ... */ }

                constexpr ALWAYS_INLINE OutBufferBase(void *ptr, size_t sz) : BaseType(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }
                constexpr ALWAYS_INLINE OutBufferBase(u8 *ptr, size_t sz) : BaseType(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }
        };

        template<u32 ExtraAttributes = 0>
        class InBufferImpl : public InBufferBase {
            public:
                using BaseType = InBufferBase;
                static constexpr u32 AdditionalAttributes = BaseType::AdditionalAttributes |
                                                            ExtraAttributes;
            public:
                constexpr ALWAYS_INLINE InBufferImpl() : BaseType() { /* ... */ }
                constexpr ALWAYS_INLINE InBufferImpl(const tipc::PointerAndSize &pas) : BaseType(pas) { /* ... */ }
                constexpr ALWAYS_INLINE InBufferImpl(uintptr_t ptr, size_t sz) : BaseType(ptr, sz) { /* ... */ }

                constexpr ALWAYS_INLINE InBufferImpl(const void *ptr, size_t sz) : BaseType(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }
                constexpr ALWAYS_INLINE InBufferImpl(const u8 *ptr, size_t sz) : BaseType(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }

                constexpr ALWAYS_INLINE const u8 *GetPointer() const {
                    return reinterpret_cast<const u8 *>(this->GetAddressImpl());
                }

                constexpr ALWAYS_INLINE size_t GetSize() const {
                    return this->GetSizeImpl<u8>();
                }
        };

        template<u32 ExtraAttributes = 0>
        class OutBufferImpl : public OutBufferBase {
            public:
                using BaseType = OutBufferBase;
                static constexpr u32 AdditionalAttributes = BaseType::AdditionalAttributes |
                                                            ExtraAttributes;
            public:
                constexpr ALWAYS_INLINE OutBufferImpl() : BaseType() { /* ... */ }
                constexpr ALWAYS_INLINE OutBufferImpl(const tipc::PointerAndSize &pas) : BaseType(pas) { /* ... */ }
                constexpr ALWAYS_INLINE OutBufferImpl(uintptr_t ptr, size_t sz) : BaseType(ptr, sz) { /* ... */ }

                constexpr ALWAYS_INLINE OutBufferImpl(void *ptr, size_t sz) : BaseType(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }
                constexpr ALWAYS_INLINE OutBufferImpl(u8 *ptr, size_t sz) : BaseType(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }

                constexpr ALWAYS_INLINE u8 *GetPointer() const {
                    return reinterpret_cast<u8 *>(this->GetAddressImpl());
                }

                constexpr ALWAYS_INLINE size_t GetSize() const {
                    return this->GetSizeImpl<u8>();
                }
        };

        template<typename T>
        struct InArrayImpl : public InBufferBase {
            public:
                using BaseType = InBufferBase;
                static constexpr u32 AdditionalAttributes = BaseType::AdditionalAttributes;
            public:
                constexpr ALWAYS_INLINE InArrayImpl() : BaseType() { /* ... */ }
                constexpr ALWAYS_INLINE InArrayImpl(const tipc::PointerAndSize &pas) : BaseType(pas) { /* ... */ }
                constexpr ALWAYS_INLINE InArrayImpl(uintptr_t ptr, size_t sz) : BaseType(ptr, sz) { /* ... */ }

                constexpr ALWAYS_INLINE InArrayImpl(const T *ptr, size_t num_elements) : BaseType(reinterpret_cast<uintptr_t>(ptr), num_elements * sizeof(T)) { /* ... */ }

                constexpr ALWAYS_INLINE const T *GetPointer() const {
                    return reinterpret_cast<const T *>(this->GetAddressImpl());
                }

                constexpr ALWAYS_INLINE size_t GetSize() const {
                    return this->GetSizeImpl<T>();
                }

                constexpr ALWAYS_INLINE const T &operator[](size_t i) const {
                    return this->GetPointer()[i];
                }

                constexpr explicit ALWAYS_INLINE operator Span<const T>() const {
                    return {this->GetPointer(), this->GetSize()};
                }

                constexpr ALWAYS_INLINE Span<const T> ToSpan() const {
                    return {this->GetPointer(), this->GetSize()};
                }
        };

        template<typename T>
        struct OutArrayImpl : public OutBufferBase {
            public:
                using BaseType = OutBufferBase;
                static constexpr u32 AdditionalAttributes = BaseType::AdditionalAttributes;
            public:
                constexpr ALWAYS_INLINE OutArrayImpl() : BaseType() { /* ... */ }
                constexpr ALWAYS_INLINE OutArrayImpl(const tipc::PointerAndSize &pas) : BaseType(pas) { /* ... */ }
                constexpr ALWAYS_INLINE OutArrayImpl(uintptr_t ptr, size_t sz) : BaseType(ptr, sz) { /* ... */ }

                constexpr ALWAYS_INLINE OutArrayImpl(T *ptr, size_t num_elements) : BaseType(reinterpret_cast<uintptr_t>(ptr), num_elements * sizeof(T)) { /* ... */ }

                constexpr ALWAYS_INLINE T *GetPointer() const {
                    return reinterpret_cast<T *>(this->GetAddressImpl());
                }

                constexpr ALWAYS_INLINE size_t GetSize() const {
                    return this->GetSizeImpl<T>();
                }

                constexpr ALWAYS_INLINE T &operator[](size_t i) const {
                    return this->GetPointer()[i];
                }

                constexpr explicit ALWAYS_INLINE operator Span<T>() const {
                    return {this->GetPointer(), this->GetSize()};
                }

                constexpr ALWAYS_INLINE Span<T> ToSpan() const {
                    return {this->GetPointer(), this->GetSize()};
                }
        };

    }

    /* Buffer Types. */
    using InBuffer            = typename impl::InBufferImpl<>;
    // using InNonSecureBuffer   = typename impl::InBufferImpl<SfBufferAttr_HipcMapTransferAllowsNonSecure>;
    // using InNonDeviceBuffer   = typename impl::InBufferImpl<SfBufferAttr_HipcMapTransferAllowsNonDevice>;

    using OutBuffer           = typename impl::OutBufferImpl<>;
    //using OutNonSecureBuffer  = typename impl::OutBufferImpl<SfBufferAttr_HipcMapTransferAllowsNonSecure>;
    //using OutNonDeviceBuffer  = typename impl::OutBufferImpl<SfBufferAttr_HipcMapTransferAllowsNonDevice>;

    template<typename T>
    using InArray             = typename impl::InArrayImpl<T>;

    template<typename T>
    using OutArray            = typename impl::OutArrayImpl<T>;

    /* Attribute serialization structs. */
    template<typename T>
    concept IsBuffer = std::derived_from<T, impl::BufferBaseTag>;

    template<typename T> requires IsBuffer<T>
    constexpr inline u32 BufferAttributes = SfBufferAttr_HipcMapAlias | T::AdditionalAttributes;

}
