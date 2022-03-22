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

        template<UsableAtomicType T>
        constexpr ALWAYS_INLINE T ConvertToTypeForAtomic(AtomicStorage<T> s) {
            if constexpr (std::integral<T>) {
                return static_cast<T>(s);
            } else if constexpr(std::is_pointer<T>::value) {
                return reinterpret_cast<T>(s);
            } else {
                return std::bit_cast<T>(s);
            }
        }

        template<UsableAtomicType T>
        constexpr ALWAYS_INLINE AtomicStorage<T> ConvertToStorageForAtomic(T arg) {
            if constexpr (std::integral<T>) {
                return static_cast<AtomicStorage<T>>(arg);
            } else if constexpr(std::is_pointer<T>::value) {
                if (std::is_constant_evaluated() && arg == nullptr) {
                    return 0;
                }

                return reinterpret_cast<AtomicStorage<T>>(arg);
            } else {
                return std::bit_cast<AtomicStorage<T>>(arg);
            }
        }

        template<std::memory_order Order, std::unsigned_integral StorageType>
        ALWAYS_INLINE StorageType AtomicLoadImpl(volatile StorageType * const p) {
            if constexpr (Order != std::memory_order_relaxed) {
                return ::ams::util::impl::LoadAcquireForAtomic(p);
            } else {
                return *p;
            }
        }

        template<std::memory_order Order, std::unsigned_integral StorageType>
        ALWAYS_INLINE void AtomicStoreImpl(volatile StorageType * const p, const StorageType s) {
            if constexpr (Order != std::memory_order_relaxed) {
                ::ams::util::impl::StoreReleaseForAtomic(p, s);
            } else {
                *p = s;
            }
        }

        template<std::memory_order Order, std::unsigned_integral StorageType>
        ALWAYS_INLINE StorageType LoadExclusiveForAtomicByMemoryOrder(volatile StorageType * const p) {
            if constexpr (Order == std::memory_order_relaxed) {
                return ::ams::util::impl::LoadExclusiveForAtomic(p);
            }  else if constexpr (Order == std::memory_order_consume || Order == std::memory_order_acquire) {
                return ::ams::util::impl::LoadAcquireExclusiveForAtomic(p);
            } else if constexpr (Order == std::memory_order_release) {
                return ::ams::util::impl::LoadExclusiveForAtomic(p);
            } else if constexpr (Order == std::memory_order_acq_rel || Order == std::memory_order_seq_cst) {
                return ::ams::util::impl::LoadAcquireExclusiveForAtomic(p);
            } else {
                static_assert(Order != Order, "Invalid memory order");
            }
        }

        template<std::memory_order Order, std::unsigned_integral StorageType>
        ALWAYS_INLINE bool StoreExclusiveForAtomicByMemoryOrder(volatile StorageType * const p, const StorageType s) {
            if constexpr (Order == std::memory_order_relaxed) {
                return ::ams::util::impl::StoreExclusiveForAtomic(p, s);
            }  else if constexpr (Order == std::memory_order_consume || Order == std::memory_order_acquire) {
                return ::ams::util::impl::StoreExclusiveForAtomic(p, s);
            } else if constexpr (Order == std::memory_order_release) {
                return ::ams::util::impl::StoreReleaseExclusiveForAtomic(p, s);
            } else if constexpr (Order == std::memory_order_acq_rel || Order == std::memory_order_seq_cst) {
                return ::ams::util::impl::StoreReleaseExclusiveForAtomic(p, s);
            } else {
                static_assert(Order != Order, "Invalid memory order");
            }
        }

        template<std::memory_order Order, std::unsigned_integral StorageType>
        ALWAYS_INLINE StorageType AtomicExchangeImpl(volatile StorageType * const p, const StorageType s) {
            StorageType current;
            do {
                current = ::ams::util::impl::LoadExclusiveForAtomicByMemoryOrder<Order>(p);
            } while(AMS_UNLIKELY(!impl::StoreExclusiveForAtomicByMemoryOrder<Order>(p, s)));

            return current;
        }

        template<std::memory_order Order, UsableAtomicType T>
        ALWAYS_INLINE bool AtomicCompareExchangeWeakImpl(volatile AtomicStorage<T> * const p, T &expected, T desired) {
            const AtomicStorage<T> e = ::ams::util::impl::ConvertToStorageForAtomic(expected);
            const AtomicStorage<T> d = ::ams::util::impl::ConvertToStorageForAtomic(desired);

            const AtomicStorage<T> current = ::ams::util::impl::LoadExclusiveForAtomicByMemoryOrder<Order>(p);
            if (AMS_UNLIKELY(current != e)) {
                impl::ClearExclusiveForAtomic();
                expected = ::ams::util::impl::ConvertToTypeForAtomic<T>(current);
                return false;
            }

            return AMS_LIKELY(impl::StoreExclusiveForAtomicByMemoryOrder<Order>(p, d));
        }

        template<std::memory_order Order, UsableAtomicType T>
        ALWAYS_INLINE bool AtomicCompareExchangeStrongImpl(volatile AtomicStorage<T> * const p, T &expected, T desired) {
            const AtomicStorage<T> e = ::ams::util::impl::ConvertToStorageForAtomic(expected);
            const AtomicStorage<T> d = ::ams::util::impl::ConvertToStorageForAtomic(desired);

            do {
                if (const AtomicStorage<T> current = ::ams::util::impl::LoadExclusiveForAtomicByMemoryOrder<Order>(p); AMS_UNLIKELY(current != e)) {
                    impl::ClearExclusiveForAtomic();
                    expected = ::ams::util::impl::ConvertToTypeForAtomic<T>(current);
                    return false;
                }
            } while (AMS_UNLIKELY(!impl::StoreExclusiveForAtomicByMemoryOrder<Order>(p, d)));

            return true;
        }

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
                return impl::ConvertToTypeForAtomic<T>(s);
            }

            static constexpr ALWAYS_INLINE StorageType ConvertToStorage(T arg) {
                return impl::ConvertToStorageForAtomic<T>(arg);
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

            ALWAYS_INLINE operator T() const { return this->Load(); }

            template<std::memory_order Order = std::memory_order_seq_cst>
            ALWAYS_INLINE T Load() const {
                return ConvertToType(impl::AtomicLoadImpl<Order>(this->GetStoragePointer()));
            }

            template<std::memory_order Order = std::memory_order_seq_cst>
            ALWAYS_INLINE void Store(T arg) {
                return impl::AtomicStoreImpl<Order>(this->GetStoragePointer(), ConvertToStorage(arg));
            }

            template<std::memory_order Order = std::memory_order_seq_cst>
            ALWAYS_INLINE T Exchange(T arg) {
                return ConvertToType(impl::AtomicExchangeImpl<Order>(this->GetStoragePointer(), ConvertToStorage(arg)));
            }

            template<std::memory_order Order = std::memory_order_seq_cst>
            ALWAYS_INLINE bool CompareExchangeWeak(T &expected, T desired) {
                return impl::AtomicCompareExchangeWeakImpl<Order, T>(this->GetStoragePointer(), expected, desired);
            }

            template<std::memory_order Order = std::memory_order_seq_cst>
            ALWAYS_INLINE bool CompareExchangeStrong(T &expected, T desired) {
                return impl::AtomicCompareExchangeStrongImpl<Order, T>(this->GetStoragePointer(), expected, desired);
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

    template<impl::UsableAtomicType T>
    class AtomicRef {
        NON_MOVEABLE(AtomicRef);
        public:
            static constexpr size_t RequiredAlignment = std::max<size_t>(sizeof(T), alignof(T));
        private:
            using StorageType = impl::AtomicStorage<T>;
            static_assert(sizeof(StorageType)  == sizeof(T));
            static_assert(alignof(StorageType) >= alignof(T));

            static constexpr bool IsIntegral = std::integral<T>;
            static constexpr bool IsPointer  = std::is_pointer<T>::value;

            static constexpr bool HasArithmeticFunctions = IsIntegral || IsPointer;

            using DifferenceType = typename std::conditional<IsIntegral, T, typename std::conditional<IsPointer, std::ptrdiff_t, void>::type>::type;

            static constexpr ALWAYS_INLINE T ConvertToType(StorageType s) {
                return impl::ConvertToTypeForAtomic<T>(s);
            }

            static constexpr ALWAYS_INLINE StorageType ConvertToStorage(T arg) {
                return impl::ConvertToStorageForAtomic<T>(arg);
            }
        private:
            volatile StorageType * const m_p;
        private:
            ALWAYS_INLINE volatile StorageType *GetStoragePointer() const { return m_p; }
        public:
            explicit ALWAYS_INLINE AtomicRef(T &t) : m_p(reinterpret_cast<volatile StorageType *>(std::addressof(t))) { /* ... */ }
            ALWAYS_INLINE AtomicRef(const AtomicRef &) noexcept = default;

            AtomicRef() = delete;
            AtomicRef &operator=(const AtomicRef &) = delete;

            ALWAYS_INLINE T operator=(T desired) const { return const_cast<AtomicRef *>(this)->Store(desired); }

            ALWAYS_INLINE operator T() const { return this->Load(); }

            template<std::memory_order Order = std::memory_order_seq_cst>
            ALWAYS_INLINE T Load() const {
                return ConvertToType(impl::AtomicLoadImpl<Order>(this->GetStoragePointer()));
            }

            template<std::memory_order Order = std::memory_order_seq_cst>
            ALWAYS_INLINE void Store(T arg) const {
                return impl::AtomicStoreImpl<Order>(this->GetStoragePointer(), ConvertToStorage(arg));
            }

            template<std::memory_order Order = std::memory_order_seq_cst>
            ALWAYS_INLINE T Exchange(T arg) const {
                return ConvertToType(impl::AtomicExchangeImpl<Order>(this->GetStoragePointer(), ConvertToStorage(arg)));
            }

            template<std::memory_order Order = std::memory_order_seq_cst>
            ALWAYS_INLINE bool CompareExchangeWeak(T &expected, T desired) const {
                return impl::AtomicCompareExchangeWeakImpl<Order, T>(this->GetStoragePointer(), expected, desired);
            }

            template<std::memory_order Order = std::memory_order_seq_cst>
            ALWAYS_INLINE bool CompareExchangeStrong(T &expected, T desired) const {
                return impl::AtomicCompareExchangeStrongImpl<Order, T>(this->GetStoragePointer(), expected, desired);
            }

            #define AMS_UTIL_IMPL_DEFINE_ATOMIC_FETCH_OPERATE_FUNCTION(_OPERATION_, _OPERATOR_, _POINTER_ALLOWED_)                                          \
                template<bool Enable = (IsIntegral || (_POINTER_ALLOWED_ && IsPointer)), typename = typename std::enable_if<Enable, void>::type>            \
                ALWAYS_INLINE T Fetch ## _OPERATION_(DifferenceType arg) const {                                                                            \
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
                ALWAYS_INLINE T operator _OPERATOR_##=(DifferenceType arg) const {                                                                          \
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
            ALWAYS_INLINE T operator++() const { static_assert(Enable == HasArithmeticFunctions); return this->FetchAdd(1) + 1; }

            template<bool Enable = HasArithmeticFunctions, typename = typename std::enable_if<Enable, void>::type>
            ALWAYS_INLINE T operator++(int) const { static_assert(Enable == HasArithmeticFunctions); return this->FetchAdd(1); }

            template<bool Enable = HasArithmeticFunctions, typename = typename std::enable_if<Enable, void>::type>
            ALWAYS_INLINE T operator--() const { static_assert(Enable == HasArithmeticFunctions); return this->FetchSub(1) - 1; }

            template<bool Enable = HasArithmeticFunctions, typename = typename std::enable_if<Enable, void>::type>
            ALWAYS_INLINE T operator--(int) const { static_assert(Enable == HasArithmeticFunctions); return this->FetchSub(1); }
    };

}