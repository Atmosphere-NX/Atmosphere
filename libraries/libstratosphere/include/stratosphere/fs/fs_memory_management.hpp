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
#include "fs_common.hpp"

namespace ams::fs {

    using AllocateFunction   = void *(*)(size_t);
    using DeallocateFunction = void (*)(void *, size_t);

    void SetAllocator(AllocateFunction allocator, DeallocateFunction deallocator);

    namespace impl {

        void *Allocate(size_t size);
        void Deallocate(void *ptr, size_t size);

        class Deleter {
            private:
                size_t size;
            public:
                Deleter() : size() { /* ... */ }
                explicit Deleter(size_t sz) : size(sz) { /* ... */ }

                void operator()(void *ptr) const {
                    ::ams::fs::impl::Deallocate(ptr, this->size);
                }
        };

        template<typename T>
        std::unique_ptr<T, Deleter> MakeUnique() {
            static_assert(util::is_pod<T>::value);
            return std::unique_ptr<T, Deleter>(static_cast<T *>(::ams::fs::impl::Allocate(sizeof(T))), Deleter(sizeof(T)));
        }

        template<typename ArrayT>
        std::unique_ptr<ArrayT, Deleter> MakeUnique(size_t size) {
            using T = typename std::remove_extent<ArrayT>::type;

            static_assert(util::is_pod<ArrayT>::value);
            static_assert(std::is_array<ArrayT>::value);

            const size_t alloc_size = sizeof(T) * size;
            return std::unique_ptr<ArrayT, Deleter>(static_cast<T *>(::ams::fs::impl::Allocate(alloc_size)), Deleter(alloc_size));
        }

    }

}
