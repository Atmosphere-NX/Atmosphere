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

        template<UsableAtomicType T>
        using AtomicStorage = typename AtomicIntegerStorage<T>::Type;

        static_assert(std::same_as<AtomicStorage<void *>, uintptr_t>);
        static_assert(std::same_as<AtomicStorage<s8>, u8>);
        static_assert(std::same_as<AtomicStorage<u8>, u8>);
        static_assert(std::same_as<AtomicStorage<s16>, u16>);
        static_assert(std::same_as<AtomicStorage<u16>, u16>);
        static_assert(std::same_as<AtomicStorage<s32>, u32>);
        static_assert(std::same_as<AtomicStorage<u32>, u32>);
        static_assert(std::same_as<AtomicStorage<s64>, u64>);
        static_assert(std::same_as<AtomicStorage<u64>, u64>);

        ALWAYS_INLINE void ClearExclusiveForAtomic() {
            __asm__ __volatile__("clrex" ::: "memory");
        }

        #define AMS_UTIL_IMPL_DEFINE_ATOMIC_LOAD_FUNCTION(_FNAME_, _MNEMONIC_)                                                                                                                  \
            template<std::unsigned_integral T> T _FNAME_ ##ForAtomic(const volatile T *);                                                                                                       \
                                                                                                                                                                                                \
            template<> ALWAYS_INLINE u8  _FNAME_ ##ForAtomic(const volatile u8  *p) { u8  v; __asm__ __volatile__(_MNEMONIC_ "b %w[v], %[p]" : [v]"=r"(v) : [p]"Q"(*p) : "memory"); return v; } \
            template<> ALWAYS_INLINE u16 _FNAME_ ##ForAtomic(const volatile u16 *p) { u16 v; __asm__ __volatile__(_MNEMONIC_ "h %w[v], %[p]" : [v]"=r"(v) : [p]"Q"(*p) : "memory"); return v; } \
            template<> ALWAYS_INLINE u32 _FNAME_ ##ForAtomic(const volatile u32 *p) { u32 v; __asm__ __volatile__(_MNEMONIC_ "  %w[v], %[p]" : [v]"=r"(v) : [p]"Q"(*p) : "memory"); return v; } \
            template<> ALWAYS_INLINE u64 _FNAME_ ##ForAtomic(const volatile u64 *p) { u64 v; __asm__ __volatile__(_MNEMONIC_ "   %[v], %[p]" : [v]"=r"(v) : [p]"Q"(*p) : "memory"); return v; }

        AMS_UTIL_IMPL_DEFINE_ATOMIC_LOAD_FUNCTION(LoadAcquire, "ldar")
        AMS_UTIL_IMPL_DEFINE_ATOMIC_LOAD_FUNCTION(LoadExclusive, "ldxr")
        AMS_UTIL_IMPL_DEFINE_ATOMIC_LOAD_FUNCTION(LoadAcquireExclusive, "ldaxr")

        #undef AMS_UTIL_IMPL_DEFINE_ATOMIC_LOAD_FUNCTION

        template<std::unsigned_integral T> void StoreReleaseForAtomic(volatile T *, T);

        template<> ALWAYS_INLINE void StoreReleaseForAtomic(volatile u8  *p, u8  v) { __asm__ __volatile__("stlrb %w[v], %[p]" : : [v]"r"(v), [p]"Q"(*p) : "memory"); }
        template<> ALWAYS_INLINE void StoreReleaseForAtomic(volatile u16 *p, u16 v) { __asm__ __volatile__("stlrh %w[v], %[p]" : : [v]"r"(v), [p]"Q"(*p) : "memory"); }
        template<> ALWAYS_INLINE void StoreReleaseForAtomic(volatile u32 *p, u32 v) { __asm__ __volatile__("stlr  %w[v], %[p]" : : [v]"r"(v), [p]"Q"(*p) : "memory"); }
        template<> ALWAYS_INLINE void StoreReleaseForAtomic(volatile u64 *p, u64 v) { __asm__ __volatile__("stlr   %[v], %[p]" : : [v]"r"(v), [p]"Q"(*p) : "memory"); }

        #define AMS_UTIL_IMPL_DEFINE_ATOMIC_STORE_EXCLUSIVE_FUNCTION(_FNAME_, _MNEMONIC_)                                                                                                                                                          \
            template<std::unsigned_integral T> bool _FNAME_ ##ForAtomic(volatile T *, T);                                                                                                                                                          \
                                                                                                                                                                                                                                                   \
            template<> ALWAYS_INLINE bool _FNAME_ ##ForAtomic(volatile u8  *p, u8  v) { int result; __asm__ __volatile__(_MNEMONIC_ "b %w[result], %w[v], %[p]" : [result]"=&r"(result) : [v]"r"(v), [p]"Q"(*p) : "memory"); return result == 0; } \
            template<> ALWAYS_INLINE bool _FNAME_ ##ForAtomic(volatile u16 *p, u16 v) { int result; __asm__ __volatile__(_MNEMONIC_ "h %w[result], %w[v], %[p]" : [result]"=&r"(result) : [v]"r"(v), [p]"Q"(*p) : "memory"); return result == 0; } \
            template<> ALWAYS_INLINE bool _FNAME_ ##ForAtomic(volatile u32 *p, u32 v) { int result; __asm__ __volatile__(_MNEMONIC_ "  %w[result], %w[v], %[p]" : [result]"=&r"(result) : [v]"r"(v), [p]"Q"(*p) : "memory"); return result == 0; } \
            template<> ALWAYS_INLINE bool _FNAME_ ##ForAtomic(volatile u64 *p, u64 v) { int result; __asm__ __volatile__(_MNEMONIC_ "  %w[result],  %[v], %[p]" : [result]"=&r"(result) : [v]"r"(v), [p]"Q"(*p) : "memory"); return result == 0; }

        AMS_UTIL_IMPL_DEFINE_ATOMIC_STORE_EXCLUSIVE_FUNCTION(StoreExclusive, "stxr")
        AMS_UTIL_IMPL_DEFINE_ATOMIC_STORE_EXCLUSIVE_FUNCTION(StoreReleaseExclusive, "stlxr")

        #undef AMS_UTIL_IMPL_DEFINE_ATOMIC_STORE_EXCLUSIVE_FUNCTION

    }

    template<impl::UsableAtomicType T>
    class Atomic {
        NON_COPYABLE(Atomic);
        NON_MOVEABLE(Atomic);
        private:
            using StorageType = impl::AtomicStorage<T>;

            static constexpr bool IsIntegral = std::integral<T>;
            static constexpr bool IsPointer  = std::is_pointer<T>::value;

            static constexpr bool HasArithmeticFunctions = IsIntegral || IsPointer;

            using DifferenceType = typename std::conditional<IsIntegral, T, typename std::conditional<IsPointer, std::ptrdiff_t, void>::type>::type;

            static constexpr ALWAYS_INLINE T ConvertToType(StorageType s) {
                if constexpr (std::integral<T>) {
                    return static_cast<T>(s);
                } else if constexpr(std::is_pointer<T>::value) {
                    return reinterpret_cast<T>(s);
                } else {
                    return std::bit_cast<T>(s);
                }
            }

            static constexpr ALWAYS_INLINE StorageType ConvertToStorage(T arg) {
                if constexpr (std::integral<T>) {
                    return static_cast<StorageType>(arg);
                } else if constexpr(std::is_pointer<T>::value) {
                    if (std::is_constant_evaluated() && arg == nullptr) {
                        return 0;
                    }

                    return reinterpret_cast<StorageType>(arg);
                } else {
                    return std::bit_cast<StorageType>(arg);
                }
            }
        private:
            StorageType m_v;
        private:
            ALWAYS_INLINE       volatile StorageType *GetStoragePointer()       { return reinterpret_cast<      volatile StorageType *>(std::addressof(m_v)); }
            ALWAYS_INLINE const volatile StorageType *GetStoragePointer() const { return reinterpret_cast<const volatile StorageType *>(std::addressof(m_v)); }
        public:
            ALWAYS_INLINE Atomic() { /* ... */ }
            constexpr ALWAYS_INLINE Atomic(T v) : m_v(ConvertToStorage(v)) { /* ... */ }

            constexpr ALWAYS_INLINE T operator=(T desired) {
                if (std::is_constant_evaluated()) {
                    m_v = ConvertToStorage(desired);
                } else {
                    this->Store(desired);
                }
                return desired;
            }

            template<std::memory_order Order = std::memory_order_seq_cst>
            ALWAYS_INLINE T Load() const {
                if constexpr (Order != std::memory_order_relaxed) {
                    return ConvertToType(impl::LoadAcquireForAtomic(this->GetStoragePointer()));
                } else {
                    return ConvertToType(*this->GetStoragePointer());
                }
            }

            template<std::memory_order Order = std::memory_order_seq_cst>
            ALWAYS_INLINE void Store(T arg) {
                if constexpr (Order != std::memory_order_relaxed) {
                    impl::StoreReleaseForAtomic(this->GetStoragePointer(), ConvertToStorage(arg));
                } else {
                    *this->GetStoragePointer() = ConvertToStorage(arg);
                }
            }

            template<std::memory_order Order = std::memory_order_seq_cst>
            ALWAYS_INLINE T Exchange(T arg) {
                volatile StorageType * const p = this->GetStoragePointer();
                const StorageType s = ConvertToStorage(arg);

                StorageType current;

                if constexpr (Order == std::memory_order_relaxed) {
                    do {
                        current = impl::LoadExclusiveForAtomic(p);
                    } while (AMS_UNLIKELY(!impl::StoreExclusiveForAtomic(p, s)));
                } else if constexpr (Order == std::memory_order_consume || Order == std::memory_order_acquire) {
                    do {
                        current = impl::LoadAcquireExclusiveForAtomic(p);
                    } while (AMS_UNLIKELY(!impl::StoreExclusiveForAtomic(p, s)));
                } else if constexpr (Order == std::memory_order_release) {
                    do {
                        current = impl::LoadExclusiveForAtomic(p);
                    } while (AMS_UNLIKELY(!impl::StoreReleaseExclusiveForAtomic(p, s)));
                } else if constexpr (Order == std::memory_order_acq_rel || Order == std::memory_order_seq_cst) {
                    do {
                        current = impl::LoadAcquireExclusiveForAtomic(p);
                    } while (AMS_UNLIKELY(!impl::StoreReleaseExclusiveForAtomic(p, s)));
                } else {
                    static_assert(Order != Order, "Invalid memory order");
                }

                return current;
            }

            template<std::memory_order Order = std::memory_order_seq_cst>
            ALWAYS_INLINE bool CompareExchangeWeak(T &expected, T desired) {
                volatile StorageType * const p = this->GetStoragePointer();
                const StorageType e = ConvertToStorage(expected);
                const StorageType d = ConvertToStorage(desired);

                if constexpr (Order == std::memory_order_relaxed) {
                    const StorageType current = impl::LoadExclusiveForAtomic(p);
                    if (AMS_UNLIKELY(current != e)) {
                        impl::ClearExclusiveForAtomic();
                        expected = ConvertToType(current);
                        return false;
                    }

                    return AMS_LIKELY(impl::StoreExclusiveForAtomic(p, d));
                } else if constexpr (Order == std::memory_order_consume || Order == std::memory_order_acquire) {
                    const StorageType current = impl::LoadAcquireExclusiveForAtomic(p);
                    if (AMS_UNLIKELY(current != e)) {
                        impl::ClearExclusiveForAtomic();
                        expected = ConvertToType(current);
                        return false;
                    }

                    return AMS_LIKELY(impl::StoreExclusiveForAtomic(p, d));
                } else if constexpr (Order == std::memory_order_release) {
                    const StorageType current = impl::LoadExclusiveForAtomic(p);
                    if (AMS_UNLIKELY(current != e)) {
                        impl::ClearExclusiveForAtomic();
                        expected = ConvertToType(current);
                        return false;
                    }

                    return AMS_LIKELY(impl::StoreReleaseExclusiveForAtomic(p, d));
                } else if constexpr (Order == std::memory_order_acq_rel || Order == std::memory_order_seq_cst) {
                    const StorageType current = impl::LoadAcquireExclusiveForAtomic(p);
                    if (AMS_UNLIKELY(current != e)) {
                        impl::ClearExclusiveForAtomic();
                        expected = ConvertToType(current);
                        return false;
                    }

                    return AMS_LIKELY(impl::StoreReleaseExclusiveForAtomic(p, d));
                } else {
                    static_assert(Order != Order, "Invalid memory order");
                }
            }

            template<std::memory_order Order = std::memory_order_seq_cst>
            ALWAYS_INLINE bool CompareExchangeStrong(T &expected, T desired) {
                volatile StorageType * const p = this->GetStoragePointer();
                const StorageType e = ConvertToStorage(expected);
                const StorageType d = ConvertToStorage(desired);

                if constexpr (Order == std::memory_order_relaxed) {
                    StorageType current;
                    do {
                        if (current = impl::LoadExclusiveForAtomic(p); current != e) {
                            impl::ClearExclusiveForAtomic();
                            expected = ConvertToType(current);
                            return false;
                        }
                    } while (!impl::StoreExclusiveForAtomic(p, d));
                } else if constexpr (Order == std::memory_order_consume || Order == std::memory_order_acquire) {
                    StorageType current;
                    do {
                        if (current = impl::LoadAcquireExclusiveForAtomic(p); current != e) {
                            impl::ClearExclusiveForAtomic();
                            expected = ConvertToType(current);
                            return false;
                        }
                    } while (!impl::StoreExclusiveForAtomic(p, d));
                } else if constexpr (Order == std::memory_order_release) {
                    StorageType current;
                    do {
                        if (current = impl::LoadExclusiveForAtomic(p); current != e) {
                            impl::ClearExclusiveForAtomic();
                            expected = ConvertToType(current);
                            return false;
                        }
                    } while (!impl::StoreReleaseExclusiveForAtomic(p, d));
                } else if constexpr (Order == std::memory_order_acq_rel || Order == std::memory_order_seq_cst) {
                    StorageType current;
                    do {
                        if (current = impl::LoadAcquireExclusiveForAtomic(p); current != e) {
                            impl::ClearExclusiveForAtomic();
                            expected = ConvertToType(current);
                            return false;
                        }
                    } while (!impl::StoreReleaseExclusiveForAtomic(p, d));
                } else {
                    static_assert(Order != Order, "Invalid memory order");
                }

                return true;
            }

            #define AMS_UTIL_IMPL_DEFINE_ATOMIC_FETCH_OPERATE_FUNCTION(_OPERATION_, _OPERATOR_, _POINTER_ALLOWED_)                                          \
                template<bool Enable = (IsIntegral || (_POINTER_ALLOWED_ && IsPointer)), typename = typename std::enable_if<Enable, void>::type>            \
                ALWAYS_INLINE T Fetch ## _OPERATION_(DifferenceType arg) {                                                                                  \
                    static_assert(Enable == (IsIntegral || (_POINTER_ALLOWED_ && IsPointer)));                                                              \
                    volatile StorageType * const p = this->GetStoragePointer();                                                                             \
                                                                                                                                                            \
                    StorageType current;                                                                                                                    \
                    do {                                                                                                                                    \
                        current = impl::LoadAcquireExclusiveForAtomic<StorageType>(p);                                                                      \
                    } while (AMS_UNLIKELY(!impl::StoreReleaseExclusiveForAtomic<StorageType>(p, ConvertToStorage(ConvertToType(current) _OPERATOR_ arg)))); \
                    return ConvertToType(current);                                                                                                          \
                }                                                                                                                                           \
                                                                                                                                                            \
                template<bool Enable = (IsIntegral || (_POINTER_ALLOWED_ && IsPointer)), typename = typename std::enable_if<Enable, void>::type>            \
                ALWAYS_INLINE T operator _OPERATOR_##=(DifferenceType arg) {                                                                                \
                    static_assert(Enable == (IsIntegral || (_POINTER_ALLOWED_ && IsPointer)));                                                              \
                    return this->Fetch ## _OPERATION_(arg) _OPERATOR_ arg;                                                                                  \
                }

            AMS_UTIL_IMPL_DEFINE_ATOMIC_FETCH_OPERATE_FUNCTION(Add, +, true)
            AMS_UTIL_IMPL_DEFINE_ATOMIC_FETCH_OPERATE_FUNCTION(Sub, -, true)
            AMS_UTIL_IMPL_DEFINE_ATOMIC_FETCH_OPERATE_FUNCTION(And, &, false)
            AMS_UTIL_IMPL_DEFINE_ATOMIC_FETCH_OPERATE_FUNCTION(Or,  |, false)
            AMS_UTIL_IMPL_DEFINE_ATOMIC_FETCH_OPERATE_FUNCTION(Xor, ^, false)

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