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

namespace ams {

    constexpr inline size_t DefaultAlignment = /*alignof(max_align_t)*/ 0x8;

    using AllocateFunction                    = void *(*)(size_t);
    using AllocateFunctionWithUserData        = void *(*)(size_t, void *);
    using AlignedAllocateFunction             = void *(*)(size_t, size_t);
    using AlignedAllocateFunctionWithUserData = void *(*)(size_t, size_t, void *);
    using DeallocateFunction                  = void  (*)(void *, size_t);
    using FreeFunction                        = void  (*)(void *);
    using FreeFunctionWithUserData            = void  (*)(void *, void *);

    class MemoryResource {
        public:
            ALWAYS_INLINE void *allocate(size_t size, size_t alignment = DefaultAlignment) {
                return this->AllocateImpl(size, alignment);
            }
            ALWAYS_INLINE void deallocate(void *buffer, size_t size, size_t alignment = DefaultAlignment) {
                this->DeallocateImpl(buffer, size, alignment);
            }
            ALWAYS_INLINE bool is_equal(const MemoryResource &resource) const {
                return this->IsEqualImpl(resource);
            }
            ALWAYS_INLINE void *Allocate(size_t size, size_t alignment = DefaultAlignment) {
                return this->AllocateImpl(size, alignment);
            }
            ALWAYS_INLINE void Deallocate(void *buffer, size_t size, size_t alignment = DefaultAlignment) {
                this->DeallocateImpl(buffer, size, alignment);
            }
            ALWAYS_INLINE bool IsEqual(const MemoryResource &resource) const {
                return this->IsEqualImpl(resource);
            }
        public:
            constexpr ~MemoryResource() = default;
        protected:
            virtual void *AllocateImpl(size_t size, size_t alignment) = 0;
            virtual void DeallocateImpl(void *buffer, size_t size, size_t alignment) = 0;
            virtual bool IsEqualImpl(const MemoryResource &resource) const = 0;
    };

}
