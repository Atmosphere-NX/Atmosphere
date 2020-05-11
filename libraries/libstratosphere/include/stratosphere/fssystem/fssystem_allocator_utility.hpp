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
#include <vapours.hpp>

namespace ams::fssystem {

    using AllocateFunction   = void *(*)(size_t size);
    using DeallocateFunction = void (*)(void *ptr, size_t size);

    void InitializeAllocator(AllocateFunction allocate_func, DeallocateFunction deallocate_func);

    void *Allocate(size_t size);
    void Deallocate(void *ptr, size_t size);

    template<typename T>
    class StdAllocator : public std::allocator<T> {
        public:
            StdAllocator() { /* ... */ }
            StdAllocator(const StdAllocator &) { /* ... */ }
            template<class U>
            StdAllocator(const StdAllocator<U> &) { /* ... */ }

            template<typename U>
            struct rebind {
                using other = StdAllocator<U>;
            };

            T *Allocate(size_t size, const T *hint = nullptr) {
                return static_cast<T *>(::ams::fssystem::Allocate(sizeof(T) * size));
            }

            void Deallocate(T *ptr, size_t size) {
                return ::ams::fssystem::Deallocate(ptr, sizeof(T) * size);
            }

            ALWAYS_INLINE T *allocate(size_t size, const T *hint = nullptr) { return this->Allocate(size, hint); }
            ALWAYS_INLINE void deallocate(T *ptr, size_t size) { return this->Deallocate(ptr, size); }
    };

    template<typename T>
    std::shared_ptr<T> AllocateShared() {
        return std::allocate_shared<T>(StdAllocator<T>{});
    }

    template<typename T, typename... Args>
    std::shared_ptr<T> AllocateShared(Args &&... args) {
        return std::allocate_shared<T>(StdAllocator<T>{}, std::forward<Args>(args)...);
    }

}
