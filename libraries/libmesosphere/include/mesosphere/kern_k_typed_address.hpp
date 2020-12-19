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
#include <mesosphere/kern_common.hpp>

namespace ams::kern {

#ifndef MESOSPHERE_DISABLE_TYPED_ADDRESSES

    template<bool Virtual, typename T>
    class KTypedAddress {
        private:
            uintptr_t m_address;
        public:
            /* Constructors. */
            constexpr ALWAYS_INLINE KTypedAddress() : m_address(0) { /* ... */ }
            constexpr ALWAYS_INLINE KTypedAddress(uintptr_t a) : m_address(a) { /* ... */ }
            template<typename U>
            constexpr ALWAYS_INLINE explicit KTypedAddress(U *ptr) : m_address(reinterpret_cast<uintptr_t>(ptr)) { /* ... */ }

            /* Copy constructor. */
            constexpr ALWAYS_INLINE KTypedAddress(const KTypedAddress &rhs) = default;

            /* Assignment operator. */
            constexpr ALWAYS_INLINE KTypedAddress &operator=(const KTypedAddress &rhs) = default;

            /* Arithmetic operators. */
            template<typename I>
            constexpr ALWAYS_INLINE KTypedAddress operator+(I rhs) const {
                static_assert(std::is_integral<I>::value);
                return m_address + rhs;
            }

            template<typename I>
            constexpr ALWAYS_INLINE KTypedAddress operator-(I rhs) const {
                static_assert(std::is_integral<I>::value);
                return m_address - rhs;
            }

            constexpr ALWAYS_INLINE ptrdiff_t operator-(KTypedAddress rhs) const {
                return m_address - rhs.m_address;
            }

            template<typename I>
            constexpr ALWAYS_INLINE KTypedAddress operator+=(I rhs) {
                static_assert(std::is_integral<I>::value);
                m_address += rhs;
                return *this;
            }

            template<typename I>
            constexpr ALWAYS_INLINE KTypedAddress operator-=(I rhs) {
                static_assert(std::is_integral<I>::value);
                m_address -= rhs;
                return *this;
            }

            /* Logical operators. */
            constexpr ALWAYS_INLINE uintptr_t operator&(uintptr_t mask) const {
                return m_address & mask;
            }

            constexpr ALWAYS_INLINE uintptr_t operator|(uintptr_t mask) const {
                return m_address | mask;
            }

            constexpr ALWAYS_INLINE uintptr_t operator<<(int shift) const {
                return m_address << shift;
            }

            constexpr ALWAYS_INLINE uintptr_t operator>>(int shift) const {
                return m_address >> shift;
            }

            template<typename U>
            constexpr ALWAYS_INLINE size_t operator/(U size) const { return m_address / size; }

            /* constexpr ALWAYS_INLINE uintptr_t operator%(U align) const { return m_address % align; } */

            /* Comparison operators. */
            constexpr ALWAYS_INLINE bool operator==(KTypedAddress rhs) const {
                return m_address == rhs.m_address;
            }

            constexpr ALWAYS_INLINE bool operator!=(KTypedAddress rhs) const {
                return m_address != rhs.m_address;
            }

            constexpr ALWAYS_INLINE bool operator<(KTypedAddress rhs) const {
                return m_address < rhs.m_address;
            }

            constexpr ALWAYS_INLINE bool operator<=(KTypedAddress rhs) const {
                return m_address <= rhs.m_address;
            }

            constexpr ALWAYS_INLINE bool operator>(KTypedAddress rhs) const {
                return m_address > rhs.m_address;
            }

            constexpr ALWAYS_INLINE bool operator>=(KTypedAddress rhs) const {
                return m_address >= rhs.m_address;
            }

            /* For convenience, also define comparison operators versus uintptr_t. */
            constexpr ALWAYS_INLINE bool operator==(uintptr_t rhs) const {
                return m_address == rhs;
            }

            constexpr ALWAYS_INLINE bool operator!=(uintptr_t rhs) const {
                return m_address != rhs;
            }

            /* Allow getting the address explicitly, for use in accessors. */
            constexpr ALWAYS_INLINE uintptr_t GetValue() const {
                return m_address;
            }

    };

    struct KPhysicalAddressTag{};
    struct KVirtualAddressTag{};
    struct KProcessAddressTag{};

    using KPhysicalAddress = KTypedAddress<false, KPhysicalAddressTag>;
    using KVirtualAddress  = KTypedAddress<true,  KVirtualAddressTag>;
    using KProcessAddress  = KTypedAddress<true,  KProcessAddressTag>;

    /* Define accessors. */
    template<bool Virtual, typename T>
    constexpr ALWAYS_INLINE uintptr_t GetInteger(KTypedAddress<Virtual, T> address) {
        return address.GetValue();
    }

    template<typename T, typename U>
    constexpr ALWAYS_INLINE T *GetPointer(KTypedAddress<true, U> address) {
        return CONST_FOLD(reinterpret_cast<T *>(address.GetValue()));
    }

    template<typename T>
    constexpr ALWAYS_INLINE void *GetVoidPointer(KTypedAddress<true, T> address) {
        return CONST_FOLD(reinterpret_cast<void *>(address.GetValue()));
    }

#else

    /* Plausibly, we may not want compiler overhead from using strongly typed addresses. */
    /* In this case, we should just use uintptr_t. */
    using KPhysicalAddress = uintptr_t;
    using KVirtualAddress  = uintptr_t;
    using KProcessAddress  = uintptr_t;

    /* Define accessors. */
    constexpr ALWAYS_INLINE uintptr_t GetInteger(uintptr_t address) {
        return address;
    }

    template<typename T>
    constexpr ALWAYS_INLINE T *GetPointer(uintptr_t address) {
        return CONST_FOLD(reinterpret_cast<T *>(address));
    }

    template<typename T>
    constexpr ALWAYS_INLINE void *GetVoidPointer(uintptr_t address) {
        return CONST_FOLD(reinterpret_cast<void *>(address));
    }

#endif

    template<typename T>
    concept IsKTypedAddress = std::same_as<T, KPhysicalAddress> || std::same_as<T, KVirtualAddress> || std::same_as<T, KProcessAddress>;

    template<typename T>
    constexpr inline T Null = [] {
        if constexpr (std::is_same<T, uintptr_t>::value) {
            return 0;
        } else {
            static_assert(std::is_same<T, KPhysicalAddress>::value ||
                          std::is_same<T, KVirtualAddress>::value  ||
                          std::is_same<T, KProcessAddress>::value);
            return T(0);
        }
    }();

    /* Basic type validations. */
    static_assert(sizeof(KPhysicalAddress) == sizeof(uintptr_t));
    static_assert(sizeof(KVirtualAddress)  == sizeof(uintptr_t));
    static_assert(sizeof(KProcessAddress)  == sizeof(uintptr_t));

    static_assert(std::is_trivially_copyable<KPhysicalAddress>::value);
    static_assert(std::is_trivially_copyable<KVirtualAddress>::value);
    static_assert(std::is_trivially_copyable<KProcessAddress>::value);

    static_assert(std::is_trivially_copy_constructible<KPhysicalAddress>::value);
    static_assert(std::is_trivially_copy_constructible<KVirtualAddress>::value);
    static_assert(std::is_trivially_copy_constructible<KProcessAddress>::value);

    static_assert(std::is_trivially_move_constructible<KPhysicalAddress>::value);
    static_assert(std::is_trivially_move_constructible<KVirtualAddress>::value);
    static_assert(std::is_trivially_move_constructible<KProcessAddress>::value);

    static_assert(std::is_trivially_copy_assignable<KPhysicalAddress>::value);
    static_assert(std::is_trivially_copy_assignable<KVirtualAddress>::value);
    static_assert(std::is_trivially_copy_assignable<KProcessAddress>::value);

    static_assert(std::is_trivially_move_assignable<KPhysicalAddress>::value);
    static_assert(std::is_trivially_move_assignable<KVirtualAddress>::value);
    static_assert(std::is_trivially_move_assignable<KProcessAddress>::value);

    static_assert(std::is_trivially_destructible<KPhysicalAddress>::value);
    static_assert(std::is_trivially_destructible<KVirtualAddress>::value);
    static_assert(std::is_trivially_destructible<KProcessAddress>::value);

    static_assert(Null<uintptr_t>        == 0);
    static_assert(Null<KPhysicalAddress> == Null<uintptr_t>);
    static_assert(Null<KVirtualAddress>  == Null<uintptr_t>);
    static_assert(Null<KProcessAddress>  == Null<uintptr_t>);

    /* Constructor/assignment validations. */
    static_assert([]{ const KPhysicalAddress a(5); KPhysicalAddress b(a); return b; }() == KPhysicalAddress(5));
    static_assert([]{ const KPhysicalAddress a(5); KPhysicalAddress b(10); b = a; return b; }() == KPhysicalAddress(5));

    /* Arithmetic validations. */
    static_assert(KPhysicalAddress(10) + 5 == KPhysicalAddress(15));
    static_assert(KPhysicalAddress(10) - 5 == KPhysicalAddress(5));
    static_assert([]{ KPhysicalAddress v(10); v += 5; return v; }() == KPhysicalAddress(15));
    static_assert([]{ KPhysicalAddress v(10); v -= 5; return v; }() == KPhysicalAddress(5));

    /* Logical validations. */
    static_assert((KPhysicalAddress(0b11111111) >> 1)         == 0b01111111);
    static_assert((KPhysicalAddress(0b10101010) >> 1)         == 0b01010101);
    static_assert((KPhysicalAddress(0b11111111) << 1)        == 0b111111110);
    static_assert((KPhysicalAddress(0b01010101) << 1)         == 0b10101010);
    static_assert((KPhysicalAddress(0b11111111) & 0b01010101) == 0b01010101);
    static_assert((KPhysicalAddress(0b11111111) & 0b10101010) == 0b10101010);
    static_assert((KPhysicalAddress(0b01010101) & 0b10101010) == 0b00000000);
    static_assert((KPhysicalAddress(0b00000000) | 0b01010101) == 0b01010101);
    static_assert((KPhysicalAddress(0b11111111) | 0b01010101) == 0b11111111);
    static_assert((KPhysicalAddress(0b10101010) | 0b01010101) == 0b11111111);

    /* Comparisons. */
    static_assert(KPhysicalAddress(0) == KPhysicalAddress(0));
    static_assert(KPhysicalAddress(0) != KPhysicalAddress(1));
    static_assert(KPhysicalAddress(0) <  KPhysicalAddress(1));
    static_assert(KPhysicalAddress(0) <= KPhysicalAddress(1));
    static_assert(KPhysicalAddress(1) >  KPhysicalAddress(0));
    static_assert(KPhysicalAddress(1) >= KPhysicalAddress(0));

    static_assert(!(KPhysicalAddress(0) == KPhysicalAddress(1)));
    static_assert(!(KPhysicalAddress(0) != KPhysicalAddress(0)));
    static_assert(!(KPhysicalAddress(1) <  KPhysicalAddress(0)));
    static_assert(!(KPhysicalAddress(1) <= KPhysicalAddress(0)));
    static_assert(!(KPhysicalAddress(0) >  KPhysicalAddress(1)));
    static_assert(!(KPhysicalAddress(0) >= KPhysicalAddress(1)));

    /* Accessors. */
    static_assert(15 == GetInteger(KPhysicalAddress(15)));
    static_assert(0  == GetInteger(Null<KPhysicalAddress>));

}
