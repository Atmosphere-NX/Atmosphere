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
#include <vapours.hpp>

/* Forward declare ams::fs allocate shared. */
namespace ams::fs::impl {

    template<typename T, template<typename, typename> class AllocatorTemplateT, typename Impl, typename... Args>
    std::shared_ptr<T> AllocateSharedImpl(Args &&... args);

}

namespace ams::fssystem {

    /* ACCURATE_TO_VERSION: Unknown */
    using AllocateFunction   = void *(*)(size_t size);
    using DeallocateFunction = void (*)(void *ptr, size_t size);

    void InitializeAllocator(AllocateFunction allocate_func, DeallocateFunction deallocate_func);
    void InitializeAllocatorForSystem(AllocateFunction allocate_func, DeallocateFunction deallocate_func);

    void *Allocate(size_t size);
    void Deallocate(void *ptr, size_t size);

    namespace impl {

        template<bool ForSystem>
        class AllocatorFunctionSet {
            public:
                static void *Allocate(size_t size);
                static void *AllocateUnsafe(size_t size);

                static void Deallocate(void *ptr, size_t size);
                static void DeallocateUnsafe(void *ptr, size_t size);

                static void LockAllocatorMutex();
                static void UnlockAllocatorMutex();
        };

        using AllocatorFunctionSetForNormal = AllocatorFunctionSet<false>;
        using AllocatorFunctionSetForSystem = AllocatorFunctionSet<true>;

        template<typename T, typename Impl, bool RequireNonNull, bool AllocateWhileLocked>
        class AllocatorTemplate : public std::allocator<T> {
            public:
                template<typename U>
                struct rebind {
                    using other = AllocatorTemplate<U, Impl, RequireNonNull, AllocateWhileLocked>;
                };
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
                AllocatorTemplate() { /* ... */ }

                template<typename U>
                AllocatorTemplate(const AllocatorTemplate<U, Impl, RequireNonNull, AllocateWhileLocked> &) { /* ... */ }

                [[nodiscard]] T *allocate(::std::size_t n) {
                    auto * const p = AllocateImpl(n);
                    if constexpr (RequireNonNull) {
                        AMS_ABORT_UNLESS(p != nullptr);
                    }
                    return p;
                }

                void deallocate(T *p, ::std::size_t n) {
                    Impl::Deallocate(p, sizeof(T) * n);
                }
        };

        template<typename T, typename Impl>
        using AllocatorTemplateForAllocateShared = AllocatorTemplate<T, Impl, true, true>;

    }

    template<typename T, typename... Args>
    std::shared_ptr<T> AllocateShared(Args &&... args) {
        return ::ams::fs::impl::AllocateSharedImpl<T, impl::AllocatorTemplateForAllocateShared, impl::AllocatorFunctionSetForNormal>(std::forward<Args>(args)...);
    }

    template<typename TImpl, typename ErrorResult, typename TIntf, typename... Args>
    Result AllocateSharedForSystem(std::shared_ptr<TIntf> *out, Args &&... args) {
        /* Allocate the object. */
        auto p = ::ams::fs::impl::AllocateSharedImpl<TImpl, impl::AllocatorTemplateForAllocateShared, impl::AllocatorFunctionSetForSystem>(std::forward<Args>(args)...);

        /* Check that allocation succeeded. */
        R_UNLESS(p != nullptr, ErrorResult());

        /* Return the allocated object. */
        *out = std::move(p);
        R_SUCCEED();
    }

}
