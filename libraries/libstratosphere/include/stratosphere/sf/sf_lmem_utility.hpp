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
#include <stratosphere/lmem.hpp>

namespace ams::sf {

    class ExpHeapMemoryResource : public MemoryResource {
        private:
            lmem::HeapHandle handle;
        public:
            constexpr ExpHeapMemoryResource() : handle() { /* ... */ }
            constexpr explicit ExpHeapMemoryResource(lmem::HeapHandle h) : handle(h) { /* ... */ }

            void Attach(lmem::HeapHandle h) {
                AMS_ABORT_UNLESS(this->handle == lmem::HeapHandle());
                this->handle = h;
            }

            lmem::HeapHandle GetHandle() const { return this->handle; }
        private:
            virtual void *AllocateImpl(size_t size, size_t alignment) override {
                return lmem::AllocateFromExpHeap(this->handle, size, static_cast<int>(alignment));
            }

            virtual void DeallocateImpl(void *buffer, size_t size, size_t alignment) override {
                return lmem::FreeToExpHeap(this->handle, buffer);
            }

            virtual bool IsEqualImpl(const MemoryResource &resource) const {
                return this == std::addressof(resource);
            }
    };

    class UnitHeapMemoryResource : public MemoryResource {
        private:
            lmem::HeapHandle handle;
        public:
            constexpr UnitHeapMemoryResource() : handle() { /* ... */ }
            constexpr explicit UnitHeapMemoryResource(lmem::HeapHandle h) : handle(h) { /* ... */ }

            void Attach(lmem::HeapHandle h) {
                AMS_ABORT_UNLESS(this->handle == lmem::HeapHandle());
                this->handle = h;
            }

            lmem::HeapHandle GetHandle() const { return this->handle; }
        private:
            virtual void *AllocateImpl(size_t size, size_t alignment) override {
                AMS_ASSERT(size <= lmem::GetUnitHeapUnitSize(this->handle));
                AMS_ASSERT(alignment <= static_cast<size_t>(lmem::GetUnitHeapAlignment(this->handle)));
                return lmem::AllocateFromUnitHeap(this->handle);
            }

            virtual void DeallocateImpl(void *buffer, size_t size, size_t alignment) override {
                return lmem::FreeToUnitHeap(this->handle, buffer);
            }

            virtual bool IsEqualImpl(const MemoryResource &resource) const {
                return this == std::addressof(resource);
            }
    };

}