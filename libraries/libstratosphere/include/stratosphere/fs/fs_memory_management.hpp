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

    }

    template<typename T, typename... Args>
    std::shared_ptr<T> AllocateShared(Args &&... args) {
        return ::ams::fs::impl::AllocateSharedImpl<T, ::ams::fs::impl::AllocatorTemplateForAllocateShared, ::ams::fs::impl::AllocatorImpl>(std::forward<Args>(args)...);
    }

}
