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
#include "sf_buffer_tags.hpp"

namespace ams::sf {

    enum class BufferTransferMode {
        MapAlias,
        Pointer,
        AutoSelect,
    };

    namespace impl {

        /* Buffer utilities. */
        struct BufferBaseTag{};

        template<BufferTransferMode TransferMode>
        constexpr inline u32 BufferTransferModeAttributes = [] {
            if constexpr (TransferMode == BufferTransferMode::MapAlias) {
                return SfBufferAttr_HipcMapAlias;
            } else if constexpr (TransferMode == BufferTransferMode::Pointer) {
                return SfBufferAttr_HipcPointer;
            } else if constexpr(TransferMode == BufferTransferMode::AutoSelect) {
                return SfBufferAttr_HipcAutoSelect;
            } else {
                static_assert(TransferMode != TransferMode, "Invalid BufferTransferMode");
            }
        }();

    }

    template<typename T>
    constexpr inline bool IsLargeData = std::is_base_of<sf::LargeData, T>::value;

    template<typename T>
    constexpr inline bool IsLargeData<Out<T>> = IsLargeData<T>;

    template<typename T>
    constexpr inline size_t LargeDataSize = sizeof(T);

    template<typename T>
    constexpr inline size_t LargeDataSize<Out<T>> = sizeof(T);

    template<typename T>
    constexpr inline BufferTransferMode PreferredTransferMode = [] {
        constexpr bool prefers_map_alias   = std::is_base_of<PrefersMapAliasTransferMode, T>::value;
        constexpr bool prefers_pointer     = std::is_base_of<PrefersPointerTransferMode, T>::value;
        constexpr bool prefers_auto_select = std::is_base_of<PrefersAutoSelectTransferMode, T>::value;
        if constexpr (prefers_map_alias) {
            static_assert(!prefers_pointer && !prefers_auto_select, "Type T must only prefer one transfer mode.");
            return BufferTransferMode::MapAlias;
        } else if constexpr (prefers_pointer) {
            static_assert(!prefers_map_alias && !prefers_auto_select, "Type T must only prefer one transfer mode.");
            return BufferTransferMode::Pointer;
        } else if constexpr (prefers_auto_select) {
            static_assert(!prefers_map_alias && !prefers_pointer, "Type T must only prefer one transfer mode.");
            return BufferTransferMode::AutoSelect;
        } else if constexpr (IsLargeData<T>) {
            return BufferTransferMode::Pointer;
        } else {
            return BufferTransferMode::MapAlias;
        }
    }();

    template<typename T>
    constexpr inline BufferTransferMode PreferredTransferMode<Out<T>> = PreferredTransferMode<T>;

    namespace impl {

        class BufferBase : public BufferBaseTag {
            public:
                static constexpr u32 AdditionalAttributes = 0;
            private:
                const cmif::PointerAndSize pas;
            protected:
                constexpr uintptr_t GetAddressImpl() const {
                    return this->pas.GetAddress();
                }

                template<typename Entry>
                constexpr inline size_t GetSizeImpl() const {
                    return this->pas.GetSize() / sizeof(Entry);
                }
            public:
                constexpr BufferBase() : pas() { /* ... */ }
                constexpr BufferBase(const cmif::PointerAndSize &_pas) : pas(_pas) { /* ... */ }
                constexpr BufferBase(uintptr_t ptr, size_t sz) : pas(ptr, sz) { /* ... */ }
        };

        class InBufferBase : public BufferBase {
            public:
                using BaseType = BufferBase;
                static constexpr u32 AdditionalAttributes = BaseType::AdditionalAttributes |
                                                            SfBufferAttr_In;
            public:
                constexpr InBufferBase() : BaseType() { /* ... */ }
                constexpr InBufferBase(const cmif::PointerAndSize &_pas) : BaseType(_pas) { /* ... */ }
                constexpr InBufferBase(uintptr_t ptr, size_t sz) : BaseType(ptr, sz) { /* ... */ }

                constexpr InBufferBase(const void *ptr, size_t sz) : BaseType(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }
                constexpr InBufferBase(const u8 *ptr, size_t sz) : BaseType(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }
        };

        class OutBufferBase : public BufferBase {
            public:
                using BaseType = BufferBase;
                static constexpr u32 AdditionalAttributes = BaseType::AdditionalAttributes |
                                                            SfBufferAttr_Out;
            public:
                constexpr OutBufferBase() : BaseType() { /* ... */ }
                constexpr OutBufferBase(const cmif::PointerAndSize &_pas) : BaseType(_pas) { /* ... */ }
                constexpr OutBufferBase(uintptr_t ptr, size_t sz) : BaseType(ptr, sz) { /* ... */ }

                constexpr OutBufferBase(void *ptr, size_t sz) : BaseType(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }
                constexpr OutBufferBase(u8 *ptr, size_t sz) : BaseType(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }
        };

        template<BufferTransferMode TMode, u32 ExtraAttributes = 0>
        class InBufferImpl : public InBufferBase {
            public:
                using BaseType = InBufferBase;
                static constexpr BufferTransferMode TransferMode = TMode;
                static constexpr u32 AdditionalAttributes = BaseType::AdditionalAttributes |
                                                            ExtraAttributes;
            public:
                constexpr InBufferImpl() : BaseType() { /* ... */ }
                constexpr InBufferImpl(const cmif::PointerAndSize &_pas) : BaseType(_pas) { /* ... */ }
                constexpr InBufferImpl(uintptr_t ptr, size_t sz) : BaseType(ptr, sz) { /* ... */ }

                constexpr InBufferImpl(const void *ptr, size_t sz) : BaseType(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }
                constexpr InBufferImpl(const u8 *ptr, size_t sz) : BaseType(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }

                constexpr const u8 *GetPointer() const {
                    return reinterpret_cast<const u8 *>(this->GetAddressImpl());
                }

                constexpr size_t GetSize() const {
                    return this->GetSizeImpl<u8>();
                }
        };

        template<BufferTransferMode TMode, u32 ExtraAttributes = 0>
        class OutBufferImpl : public OutBufferBase {
            public:
                using BaseType = OutBufferBase;
                static constexpr BufferTransferMode TransferMode = TMode;
                static constexpr u32 AdditionalAttributes = BaseType::AdditionalAttributes |
                                                            ExtraAttributes;
            public:
                constexpr OutBufferImpl() : BaseType() { /* ... */ }
                constexpr OutBufferImpl(const cmif::PointerAndSize &_pas) : BaseType(_pas) { /* ... */ }
                constexpr OutBufferImpl(uintptr_t ptr, size_t sz) : BaseType(ptr, sz) { /* ... */ }

                constexpr OutBufferImpl(void *ptr, size_t sz) : BaseType(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }
                constexpr OutBufferImpl(u8 *ptr, size_t sz) : BaseType(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }

                constexpr u8 *GetPointer() const {
                    return reinterpret_cast<u8 *>(this->GetAddressImpl());
                }

                constexpr size_t GetSize() const {
                    return this->GetSizeImpl<u8>();
                }
        };

        template<typename T, BufferTransferMode TMode = PreferredTransferMode<T>>
        struct InArrayImpl : public InBufferBase {
            public:
                using BaseType = InBufferBase;
                static constexpr BufferTransferMode TransferMode = TMode;
                static constexpr u32 AdditionalAttributes = BaseType::AdditionalAttributes;
            public:
                constexpr InArrayImpl() : BaseType() { /* ... */ }
                constexpr InArrayImpl(const cmif::PointerAndSize &_pas) : BaseType(_pas) { /* ... */ }
                constexpr InArrayImpl(const T *ptr, size_t num_elements) : BaseType(reinterpret_cast<uintptr_t>(ptr), num_elements * sizeof(T)) { /* ... */ }

                constexpr const T *GetPointer() const {
                    return reinterpret_cast<const T *>(this->GetAddressImpl());
                }

                constexpr size_t GetSize() const {
                    return this->GetSizeImpl<T>();
                }

                constexpr const T &operator[](size_t i) const {
                    return this->GetPointer()[i];
                }

                constexpr explicit operator Span<const T>() const {
                    return {this->GetPointer(), static_cast<ptrdiff_t>(this->GetSize())};
                }

                constexpr Span<const T> ToSpan() const {
                    return {this->GetPointer(), static_cast<ptrdiff_t>(this->GetSize())};
                }
        };

        template<typename T, BufferTransferMode TMode = PreferredTransferMode<T>>
        struct OutArrayImpl : public OutBufferBase {
            public:
                using BaseType = OutBufferBase;
                static constexpr BufferTransferMode TransferMode = TMode;
                static constexpr u32 AdditionalAttributes = BaseType::AdditionalAttributes;
            public:
                constexpr OutArrayImpl() : BaseType() { /* ... */ }
                constexpr OutArrayImpl(const cmif::PointerAndSize &_pas) : BaseType(_pas) { /* ... */ }
                constexpr OutArrayImpl(T *ptr, size_t num_elements) : BaseType(reinterpret_cast<uintptr_t>(ptr), num_elements * sizeof(T)) { /* ... */ }

                constexpr T *GetPointer() const {
                    return reinterpret_cast<T *>(this->GetAddressImpl());
                }

                constexpr size_t GetSize() const {
                    return this->GetSizeImpl<T>();
                }

                constexpr T &operator[](size_t i) const {
                    return this->GetPointer()[i];
                }

                constexpr explicit operator Span<T>() const {
                    return {this->GetPointer(), static_cast<ptrdiff_t>(this->GetSize())};
                }

                constexpr Span<T> ToSpan() const {
                    return {this->GetPointer(), static_cast<ptrdiff_t>(this->GetSize())};
                }
        };

    }

    /* Buffer Types. */
    using InBuffer            = typename impl::InBufferImpl<BufferTransferMode::MapAlias>;
    using InMapAliasBuffer    = typename impl::InBufferImpl<BufferTransferMode::MapAlias>;
    using InPointerBuffer     = typename impl::InBufferImpl<BufferTransferMode::Pointer>;
    using InAutoSelectBuffer  = typename impl::InBufferImpl<BufferTransferMode::AutoSelect>;
    using InNonSecureBuffer   = typename impl::InBufferImpl<BufferTransferMode::MapAlias, SfBufferAttr_HipcMapTransferAllowsNonSecure>;
    using InNonDeviceBuffer   = typename impl::InBufferImpl<BufferTransferMode::MapAlias, SfBufferAttr_HipcMapTransferAllowsNonDevice>;

    using OutBuffer           = typename impl::OutBufferImpl<BufferTransferMode::MapAlias>;
    using OutMapAliasBuffer   = typename impl::OutBufferImpl<BufferTransferMode::MapAlias>;
    using OutPointerBuffer    = typename impl::OutBufferImpl<BufferTransferMode::Pointer>;
    using OutAutoSelectBuffer = typename impl::OutBufferImpl<BufferTransferMode::AutoSelect>;
    using OutNonSecureBuffer  = typename impl::OutBufferImpl<BufferTransferMode::MapAlias, SfBufferAttr_HipcMapTransferAllowsNonSecure>;
    using OutNonDeviceBuffer  = typename impl::OutBufferImpl<BufferTransferMode::MapAlias, SfBufferAttr_HipcMapTransferAllowsNonDevice>;

    template<typename T>
    using InArray             = typename impl::InArrayImpl<T>;
    template<typename T>
    using InMapAliasArray     = typename impl::InArrayImpl<T, BufferTransferMode::MapAlias>;
    template<typename T>
    using InPointerArray      = typename impl::InArrayImpl<T, BufferTransferMode::Pointer>;
    template<typename T>
    using InAutoSelectArray   = typename impl::InArrayImpl<T, BufferTransferMode::AutoSelect>;

    template<typename T>
    using OutArray            = typename impl::OutArrayImpl<T>;
    template<typename T>
    using OutMapAliasArray    = typename impl::OutArrayImpl<T, BufferTransferMode::MapAlias>;
    template<typename T>
    using OutPointerArray     = typename impl::OutArrayImpl<T, BufferTransferMode::Pointer>;
    template<typename T>
    using OutAutoSelectArray  = typename impl::OutArrayImpl<T, BufferTransferMode::AutoSelect>;

    /* Attribute serialization structs. */
    template<typename T>
    constexpr inline bool IsBuffer = [] {
        const bool is_buffer = std::is_base_of<impl::BufferBaseTag, T>::value;
        const bool is_large_data = IsLargeData<T>;
        static_assert(!(is_buffer && is_large_data), "Invalid sf::IsBuffer state");
        return is_buffer || is_large_data;
    }();

    template<typename T>
    constexpr inline u32 BufferAttributes = [] {
        static_assert(IsBuffer<T>, "BufferAttributes requires IsBuffer");
        if constexpr (std::is_base_of<impl::BufferBaseTag, T>::value) {
            return impl::BufferTransferModeAttributes<T::TransferMode> | T::AdditionalAttributes;
        } else if constexpr (IsLargeData<T>) {
            u32 attr = SfBufferAttr_FixedSize | impl::BufferTransferModeAttributes<PreferredTransferMode<T>>;
            if constexpr (std::is_base_of<impl::OutBaseTag, T>::value) {
                attr |= SfBufferAttr_Out;
            } else {
                attr |= SfBufferAttr_In;
            }
            return attr;
        } else {
            static_assert(!std::is_same<T, T>::value, "Invalid BufferAttributes<T>");
        }
    }();

}