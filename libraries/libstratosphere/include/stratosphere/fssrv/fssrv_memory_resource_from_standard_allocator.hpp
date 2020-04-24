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
#include <stratosphere/os.hpp>

namespace ams::mem {

    class StandardAllocator;

}

namespace ams::fssrv {

    class MemoryResourceFromStandardAllocator : public ams::MemoryResource {
        private:
            mem::StandardAllocator *allocator;
            os::SdkMutex mutex;
            size_t peak_free_size;
            size_t current_free_size;
            size_t peak_allocated_size;
        public:
            explicit MemoryResourceFromStandardAllocator(mem::StandardAllocator *allocator);
        public:
            size_t GetPeakFreeSize() const { return this->peak_free_size; }
            size_t GetCurrentFreeSize() const { return this->current_free_size; }
            size_t GetPeakAllocatedSize() const { return this->peak_allocated_size; }

            void ClearPeak();
        protected:
            virtual void *AllocateImpl(size_t size, size_t align) override;
            virtual void DeallocateImpl(void *p, size_t size, size_t align) override;
            virtual bool IsEqualImpl(const MemoryResource &rhs) const override {
                return false;
            }
    };

}