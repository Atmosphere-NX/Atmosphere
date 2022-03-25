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
#include <stratosphere/fs/fs_common.hpp>

namespace ams::fs {

    /* Controls whether MakeUniqueBuffer uses a custom buffer wrapper which wraps the size inline. */
    #define AMS_FS_IMPL_MAKE_UNIQUE_BUFFER_WITH_INLINE_SIZE

    /* ACCURATE_TO_VERSION: Unknown */
    using AllocateFunction   = void *(*)(size_t);
    using DeallocateFunction = void (*)(void *, size_t);

    void SetAllocator(AllocateFunction allocator, DeallocateFunction deallocator);

    namespace impl {

        class Newable;

        void *Allocate(size_t size);
        void Deallocate(void *ptr, size_t size);

        void LockAllocatorMutex();
        void UnlockAllocatorMutex();

        void *AllocateUnsafe(size_t size);
        void DeallocateUnsafe(void *ptr, size_t size);

        class AllocatorImpl {
            public:
                static ALWAYS_INLINE void *Allocate(size_t size) { return ::ams::fs::impl::Allocate(size); }
                static ALWAYS_INLINE void *AllocateUnsafe(size_t size) { return ::ams::fs::impl::AllocateUnsafe(size); }

                static ALWAYS_INLINE void Deallocate(void *ptr, size_t size) { return ::ams::fs::impl::Deallocate(ptr, size); }
                static ALWAYS_INLINE void DeallocateUnsafe(void *ptr, size_t size) { return ::ams::fs::impl::DeallocateUnsafe(ptr, size); }

                static ALWAYS_INLINE void LockAllocatorMutex() { return ::ams::fs::impl::LockAllocatorMutex(); }
                static ALWAYS_INLINE void UnlockAllocatorMutex() { return ::ams::fs::impl::UnlockAllocatorMutex(); }
        };

        template<typename T, typename Impl, bool AllocateWhileLocked>
        class AllocatorTemplate : public std::allocator<T> {
            public:
                template<typename U>
                struct rebind {
                    using other = AllocatorTemplate<U, Impl, AllocateWhileLocked>;
                };
            private:
                bool m_allocation_failed;
            private:
                static ALWAYS_INLINE T *AllocateImpl(::std::size_t n) {
                    if constexpr (AllocateWhileLocked) {
                        auto * const p = Impl::AllocateUnsafe(sizeof(T) * n);
                        Impl::UnlockAllocatorMutex();
                        return static_cast<T *>(p);
                    } else {
                        return static_cast<T *>(Impl::Allocate(sizeof(T) * n));
                    }
                }
            public:
                AllocatorTemplate() : m_allocation_failed(false) { /* ... */ }

                template<typename U>
                AllocatorTemplate(const AllocatorTemplate<U, Impl, AllocateWhileLocked> &rhs) : m_allocation_failed(rhs.IsAllocationFailed()) { /* ... */ }

                bool IsAllocationFailed() const { return m_allocation_failed; }

                [[nodiscard]] T *allocate(::std::size_t n) {
                    auto * const p = AllocateImpl(n);
                    if (AMS_UNLIKELY(p == nullptr) && n) {
                        m_allocation_failed = true;
                    }
                    return p;
                }

                void deallocate(T *p, ::std::size_t n) {
                    Impl::Deallocate(p, sizeof(T) * n);
                }
        };

        template<typename T, typename Impl>
        using AllocatorTemplateForAllocateShared = AllocatorTemplate<T, Impl, true>;

        template<typename T, template<typename, typename> class AllocatorTemplateT, typename Impl, typename... Args>
        std::shared_ptr<T> AllocateSharedImpl(Args &&... args) {
            /* Try to allocate. */
            {
                /* Acquire exclusive access to the allocator. */
                Impl::LockAllocatorMutex();

                /* Check that we can allocate memory (using overestimate of 0x80 + sizeof(T)). */
                if (auto * const p = Impl::AllocateUnsafe(0x80 + sizeof(T)); AMS_LIKELY(p != nullptr)) {
                    /* Free the memory we allocated. */
                    Impl::DeallocateUnsafe(p, 0x80 + sizeof(T));

                    /* Get allocator type. */
                    using AllocatorType = AllocatorTemplateT<T, Impl>;

                    /* Allocate the shared pointer. */
                    return std::allocate_shared<T>(AllocatorType{}, std::forward<Args>(args)...);
                } else {
                    /* We can't allocate. */
                    Impl::UnlockAllocatorMutex();
                }
            }

            /* We failed. */
            return nullptr;
        }

        class Deleter {
            private:
                size_t m_size;
            public:
                Deleter() : m_size() { /* ... */ }
                explicit Deleter(size_t sz) : m_size(sz) { /* ... */ }

                void operator()(void *ptr) const {
                    ::ams::fs::impl::Deallocate(ptr, m_size);
                }
        };

        #if defined(AMS_FS_IMPL_MAKE_UNIQUE_BUFFER_WITH_INLINE_SIZE)
        template<std::convertible_to<u8> BufferEntryType>
        class BufferWithInlineSize final {
            static_assert(sizeof(BufferEntryType) == 1);
            NON_COPYABLE(BufferWithInlineSize);
            NON_MOVEABLE(BufferWithInlineSize);
            private:
                template<std::unsigned_integral T> static constexpr inline T EncodedSizeMask = util::IsLittleEndian() ? (static_cast<T>(3u) << (BITSIZEOF(T) - 2)) : static_cast<T>(3u);

                template<std::unsigned_integral T> static constexpr inline T EncodedSize1    = util::IsLittleEndian() ? (static_cast<T>(0u) << (BITSIZEOF(T) - 2)) : static_cast<T>(0u);
                template<std::unsigned_integral T> static constexpr inline T EncodedSize2    = util::IsLittleEndian() ? (static_cast<T>(1u) << (BITSIZEOF(T) - 2)) : static_cast<T>(1u);
                template<std::unsigned_integral T> static constexpr inline T EncodedSize4    = util::IsLittleEndian() ? (static_cast<T>(2u) << (BITSIZEOF(T) - 2)) : static_cast<T>(2u);
                template<std::unsigned_integral T> static constexpr inline T EncodedSize8    = util::IsLittleEndian() ? (static_cast<T>(3u) << (BITSIZEOF(T) - 2)) : static_cast<T>(3u);

                static constexpr inline u64 TestSize1Mask = ~((static_cast<u64>(1u) << (BITSIZEOF(u8)  - 2)) - static_cast<u64>(1));
                static constexpr inline u64 TestSize2Mask = ~((static_cast<u64>(1u) << (BITSIZEOF(u16) - 2)) - static_cast<u64>(1));
                static constexpr inline u64 TestSize4Mask = ~((static_cast<u64>(1u) << (BITSIZEOF(u32) - 2)) - static_cast<u64>(1));
                static constexpr inline u64 TestSize8Mask = ~((static_cast<u64>(1u) << (BITSIZEOF(u64) - 2)) - static_cast<u64>(1));

                template<std::unsigned_integral SizeType>
                static constexpr ALWAYS_INLINE SizeType EncodeSize(SizeType type, size_t size) noexcept {
                    if constexpr (util::IsLittleEndian()) {
                        return type | static_cast<SizeType>(size);
                    } else {
                        return type | (static_cast<SizeType>(size) << 2);
                    }
                }

                template<std::unsigned_integral SizeType>
                static constexpr ALWAYS_INLINE size_t DecodeSize(const SizeType encoded) noexcept {
                    if constexpr (util::IsLittleEndian()) {
                        /* Small optimization: 1-byte size has size type field == 0 and no shifting, can return the value directly. */
                        if constexpr (sizeof(SizeType) == 1) {
                            static_assert(EncodedSize1<SizeType> == 0);
                            return encoded;
                        } else {
                            /* On little endian, we want to mask out the high bits storing the size field. */
                            constexpr SizeType DecodedSizeMask = static_cast<SizeType>(~EncodedSizeMask<SizeType>);

                            return encoded & DecodedSizeMask;
                        }
                    } else {
                        /* On big endian, we want to shift out the low bits storing the size type field. */
                        return encoded >> 2;
                    }
                }

                template<std::unsigned_integral SizeType>
                static ALWAYS_INLINE void DeleteBufferImpl(BufferEntryType *buffer) noexcept {
                    /* Get pointer to start of allocation. */
                    SizeType *alloc = reinterpret_cast<SizeType *>(buffer) - 1;

                    /* Decode the size of the allocation. */
                    const size_t alloc_size = sizeof(SizeType) + DecodeSize<SizeType>(*alloc);

                    /* Delete the buffer. */
                    return ::ams::fs::impl::Deallocate(alloc, alloc_size);
                }

                template<std::unsigned_integral SizeType, SizeType EncodedSizeType>
                static std::unique_ptr<BufferWithInlineSize> MakeBuffer(size_t size) noexcept {
                    /* Allocate a buffer. */
                    SizeType *alloc = static_cast<SizeType *>(::ams::fs::impl::Allocate(sizeof(SizeType) + size));
                    if (AMS_UNLIKELY(alloc == nullptr)) {
                        return nullptr;
                    }

                    /* Write the encoded size. */
                    if constexpr (util::IsLittleEndian()) {
                        *alloc = EncodedSizeType | static_cast<SizeType>(size);
                    } else {
                        *alloc = EncodedSizeType | (static_cast<SizeType>(size) << 2);
                    }

                    /* Return our buffer. */
                    return std::unique_ptr<BufferWithInlineSize>(reinterpret_cast<BufferWithInlineSize *>(alloc + 1));
                }

                static void DeleteBuffer(BufferEntryType *buffer) noexcept {
                    /* Convert to u8 pointer */
                    const u8 *buffer_u8 = reinterpret_cast<const u8 *>(buffer);

                    /* Determine the storage size for the size. */
                    const auto size_type = buffer_u8[-1] & EncodedSizeMask<u8>;
                    if (size_type == EncodedSize1<u8>) {
                        return DeleteBufferImpl<u8>(buffer);
                    } else if (size_type == EncodedSize2<u8>) {
                        return DeleteBufferImpl<u16>(buffer);
                    } else if (size_type == EncodedSize4<u8>) {
                        return DeleteBufferImpl<u32>(buffer);
                    } else /* if (size_type == EncodedSize8<u8>) */ {
                        return DeleteBufferImpl<u64>(buffer);
                    }
                }
            private:
                BufferEntryType m_buffer[1];
            private:
                ALWAYS_INLINE BufferWithInlineSize() noexcept { /* ... */ }
            public:
                ALWAYS_INLINE ~BufferWithInlineSize() noexcept { /* ... */ }
            public:
                ALWAYS_INLINE operator BufferEntryType *() noexcept {
                    return m_buffer;
                }

                ALWAYS_INLINE operator const BufferEntryType *() const noexcept {
                    return m_buffer;
                }
            public:
                static ALWAYS_INLINE std::unique_ptr<BufferWithInlineSize> Make(size_t size) noexcept {
                    /* Create based on overhead size. */
                    if (!(size & TestSize1Mask)) {
                        return MakeBuffer<u8, EncodedSize1<u8>>(size);
                    } else if (!(size & TestSize2Mask)) {
                        return MakeBuffer<u16, EncodedSize2<u16>>(size);
                    } else if (!(size & TestSize4Mask)) {
                        return MakeBuffer<u32, EncodedSize4<u32>>(size);
                    } else /* if (!(size & TestSize8Mask)) */ {
                        /* Check pre-condition. */
                        AMS_ASSERT(!(size & TestSize8Mask));
                        return MakeBuffer<u64, EncodedSize8<u64>>(size);
                    }
                }
            public:
                static ALWAYS_INLINE void *operator new(size_t) noexcept { AMS_ABORT(AMS_CURRENT_FUNCTION_NAME); }

                static ALWAYS_INLINE void *operator new(size_t size, Newable *placement) noexcept { AMS_ABORT(AMS_CURRENT_FUNCTION_NAME); }

                static ALWAYS_INLINE void operator delete(void *ptr, size_t) noexcept {
                    /* Delete the buffer. */
                    DeleteBuffer(reinterpret_cast<BufferWithInlineSize *>(ptr)->m_buffer);
                }

                static void *operator new[](size_t size) noexcept = delete;
                static void operator delete[](void *ptr, size_t size) noexcept = delete;
        };
        #endif

        template<typename T>
        auto MakeUnique() {
            /* Check that we're not using MakeUnique unnecessarily. */
            static_assert(!std::derived_from<T, ::ams::fs::impl::Newable>);

            return std::unique_ptr<T, Deleter>(static_cast<T *>(::ams::fs::impl::Allocate(sizeof(T))), Deleter(sizeof(T)));
        }

        template<typename ArrayT>
        auto MakeUnique(size_t size) {
            using T = typename std::remove_extent<ArrayT>::type;

            static_assert(util::is_pod<ArrayT>::value);
            static_assert(std::is_array<ArrayT>::value);

            /* Check that we're not using MakeUnique unnecessarily. */
            static_assert(!std::derived_from<T, ::ams::fs::impl::Newable>);

            using ReturnType = std::unique_ptr<ArrayT, Deleter>;

            const size_t alloc_size = sizeof(T) * size;
            return ReturnType(static_cast<T *>(::ams::fs::impl::Allocate(alloc_size)), Deleter(alloc_size));
        }

        template<typename T>
        auto MakeUniqueBuffer(size_t size) {
            #if defined(AMS_FS_IMPL_MAKE_UNIQUE_BUFFER_WITH_INLINE_SIZE)
            return BufferWithInlineSize<T>::Make(size);
            #else
            return ::ams::fs::impl::MakeUnique<T[]>(size);
            #endif
        }

    }

    template<typename T, typename... Args>
    std::shared_ptr<T> AllocateShared(Args &&... args) {
        return ::ams::fs::impl::AllocateSharedImpl<T, ::ams::fs::impl::AllocatorTemplateForAllocateShared, ::ams::fs::impl::AllocatorImpl>(std::forward<Args>(args)...);
    }

}
