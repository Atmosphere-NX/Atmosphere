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
#include <stratosphere/sf/sf_common.hpp>
#include <stratosphere/mem.hpp>

namespace ams::sf {

    class StandardAllocatorMemoryResource : public MemoryResource {
        private:
            mem::StandardAllocator *standard_allocator;
        public:
            explicit StandardAllocatorMemoryResource(mem::StandardAllocator *sa) : standard_allocator(sa) { /* ... */ }

            mem::StandardAllocator *GetAllocator() const { return this->standard_allocator; }
        private:
            virtual void *AllocateImpl(size_t size, size_t alignment) override {
                return this->standard_allocator->Allocate(size, alignment);
            }

            virtual void DeallocateImpl(void *buffer, size_t size, size_t alignment) override {
                return this->standard_allocator->Free(buffer);
            }

            virtual bool IsEqualImpl(const MemoryResource &resource) const {
                return this == std::addressof(resource);
            }
    };

}