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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>

namespace ams::util {

    namespace impl {

        template<typename T>
        struct AtomicIntegerStorage;

        template<typename T> requires (sizeof(T) == sizeof(u8))
        struct AtomicIntegerStorage<T> {
            using Type = u8;
        };

        template<typename T> requires (sizeof(T) == sizeof(u16))
        struct AtomicIntegerStorage<T> {
            using Type = u16;
        };

        template<typename T> requires (sizeof(T) == sizeof(u32))
        struct AtomicIntegerStorage<T> {
            using Type = u32;
        };

        template<typename T> requires (sizeof(T) == sizeof(u64))
        struct AtomicIntegerStorage<T> {
            using Type = u64;
        };

        template<typename T>
        concept UsableAtomicType = (sizeof(T) <= sizeof(u64)) && !std::is_const<T>::value && !std::is_volatile<T>::value && (std::is_pointer<T>::value || requires (const T &t) {
            std::bit_cast<typename AtomicIntegerStorage<T>::Type, T>(t);
        });

    }

    template<impl::UsableAtomicType T>
    class Atomic {
        NON_COPYABLE(Atomic);
        NON_MOVEABLE(Atomic);
        private:
            static constexpr bool IsIntegral = std::integral<T>;
            static constexpr bool IsPointer  = std::is_pointer<T>::value;

            static constexpr bool HasArithmeticFunctions = IsIntegral || IsPointer;

            using DifferenceType = typename std::conditional<IsIntegral, T, typename std::conditional<IsPointer, std::ptrdiff_t, void>::type>::type;
        private:
            static_assert(std::atomic<T>::is_always_lock_free);
        private:
            std::atomic<T> m_v;
        public:
            ALWAYS_INLINE Atomic() { /* ... */ }
            constexpr ALWAYS_INLINE Atomic(T v) : m_v(v) { /* ... */ }

            ALWAYS_INLINE T operator=(T desired) {
                return (m_v = desired);
            }

            template<std::memory_order Order = std::memory_order_seq_cst>
            ALWAYS_INLINE T Load() const {
                return m_v.load(Order);
            }

            template<std::memory_order Order = std::memory_order_seq_cst>
            ALWAYS_INLINE void Store(T arg) {
                return m_v.store(Order);
            }

            template<std::memory_order Order>
            ALWAYS_INLINE T Exchange(T arg) {
                return m_v.exchange(arg, Order);
            }

            template<std::memory_order Order>
            ALWAYS_INLINE bool CompareExchangeWeak(T &expected, T desired) {
                return m_v.compare_exchange_weak(expected, desired, Order);
            }

            template<std::memory_order Order>
            ALWAYS_INLINE bool CompareExchangeStrong(T &expected, T desired) {
                return m_v.compare_exchange_strong(expected, desired, Order);
            }


            #define AMS_UTIL_IMPL_DEFINE_ATOMIC_FETCH_OPERATE_FUNCTION(_OPERATION_, _OPERATION_LOWER_, _OPERATOR_, _POINTER_ALLOWED_)            \
                template<bool Enable = (IsIntegral || (_POINTER_ALLOWED_ && IsPointer)), typename = typename std::enable_if<Enable, void>::type> \
                ALWAYS_INLINE T Fetch ## _OPERATION_(DifferenceType arg) {                                                                       \
                    static_assert(Enable == (IsIntegral || (_POINTER_ALLOWED_ && IsPointer)));                                                   \
                    return m_v.fetch_##_OPERATION_LOWER_(arg);                                                                                   \
                }                                                                                                                                \
                                                                                                                                                 \
                template<bool Enable = (IsIntegral || (_POINTER_ALLOWED_ && IsPointer)), typename = typename std::enable_if<Enable, void>::type> \
                ALWAYS_INLINE T operator _OPERATOR_##=(DifferenceType arg) {                                                                     \
                    static_assert(Enable == (IsIntegral || (_POINTER_ALLOWED_ && IsPointer)));                                                   \
                    return this->Fetch##_OPERATION_(arg) _OPERATOR_ arg;                                                                         \
                }

            AMS_UTIL_IMPL_DEFINE_ATOMIC_FETCH_OPERATE_FUNCTION(Add, add, +, true)
            AMS_UTIL_IMPL_DEFINE_ATOMIC_FETCH_OPERATE_FUNCTION(Sub, sub, -, true)
            AMS_UTIL_IMPL_DEFINE_ATOMIC_FETCH_OPERATE_FUNCTION(And, and, &, false)
            AMS_UTIL_IMPL_DEFINE_ATOMIC_FETCH_OPERATE_FUNCTION(Or,  or,  |, false)
            AMS_UTIL_IMPL_DEFINE_ATOMIC_FETCH_OPERATE_FUNCTION(Xor, xor, ^, false)

            #undef AMS_UTIL_IMPL_DEFINE_ATOMIC_FETCH_OPERATE_FUNCTION

            template<bool Enable = HasArithmeticFunctions, typename = typename std::enable_if<Enable, void>::type>
            ALWAYS_INLINE T operator++() { static_assert(Enable == HasArithmeticFunctions); return this->FetchAdd(1) + 1; }

            template<bool Enable = HasArithmeticFunctions, typename = typename std::enable_if<Enable, void>::type>
            ALWAYS_INLINE T operator++(int) { static_assert(Enable == HasArithmeticFunctions); return this->FetchAdd(1); }

            template<bool Enable = HasArithmeticFunctions, typename = typename std::enable_if<Enable, void>::type>
            ALWAYS_INLINE T operator--() { static_assert(Enable == HasArithmeticFunctions); return this->FetchSub(1) - 1; }

            template<bool Enable = HasArithmeticFunctions, typename = typename std::enable_if<Enable, void>::type>
            ALWAYS_INLINE T operator--(int) { static_assert(Enable == HasArithmeticFunctions); return this->FetchSub(1); }
    };


}