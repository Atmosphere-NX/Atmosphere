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
#include <stratosphere/fs/impl/fs_newable.hpp>

namespace ams::fssystem {

    constexpr inline size_t BufferPoolAlignment = 4_KB;
    constexpr inline size_t BufferPoolWorkSize  = 320;

    class PooledBuffer {
        NON_COPYABLE(PooledBuffer);
        private:
            char *buffer;
            size_t size;
        private:
            static size_t GetAllocatableSizeMaxCore(bool large);
        public:
            static size_t GetAllocatableSizeMax() { return GetAllocatableSizeMaxCore(false); }
            static size_t GetAllocatableParticularlyLargeSizeMax() { return GetAllocatableSizeMaxCore(true); }
        private:
            void Swap(PooledBuffer &rhs) {
                std::swap(this->buffer, rhs.buffer);
                std::swap(this->size, rhs.size);
            }
        public:
            /* Constructor/Destructor. */
            constexpr PooledBuffer() : buffer(), size() { /* ... */ }

            PooledBuffer(size_t ideal_size, size_t required_size) : buffer(), size() {
                this->Allocate(ideal_size, required_size);
            }

            ~PooledBuffer() {
                this->Deallocate();
            }

            /* Move and assignment. */
            explicit PooledBuffer(PooledBuffer &&rhs) : buffer(rhs.buffer), size(rhs.size) {
                rhs.buffer = nullptr;
                rhs.size   = 0;
            }

            PooledBuffer &operator=(PooledBuffer &&rhs) {
                PooledBuffer(std::move(rhs)).Swap(*this);
                return *this;
            }

            /* Allocation API. */
            void Allocate(size_t ideal_size, size_t required_size) {
                return this->AllocateCore(ideal_size, required_size, false);
            }

            void AllocateParticularlyLarge(size_t ideal_size, size_t required_size) {
                return this->AllocateCore(ideal_size, required_size, true);
            }

            void Shrink(size_t ideal_size);

            void Deallocate() {
                /* Shrink the buffer to empty. */
                this->Shrink(0);
                AMS_ASSERT(this->buffer == nullptr);
            }

            char *GetBuffer() const {
                AMS_ASSERT(this->buffer != nullptr);
                return this->buffer;
            }

            size_t GetSize() const {
                AMS_ASSERT(this->buffer != nullptr);
                return this->size;
            }
        private:
            void AllocateCore(size_t ideal_size, size_t required_size, bool large);
    };

    Result InitializeBufferPool(char *buffer, size_t size);
    Result InitializeBufferPool(char *buffer, size_t size, char *work, size_t work_size);

    bool IsPooledBuffer(const void *buffer);

    size_t GetPooledBufferRetriedCount();
    size_t GetPooledBufferReduceAllocationCount();
    size_t GetPooledBufferFreeSizePeak();

    void ClearPooledBufferPeak();

    void RegisterAdditionalDeviceAddress(uintptr_t address, size_t size);
    void UnregisterAdditionalDeviceAddress(uintptr_t address);
    bool IsAdditionalDeviceAddress(const void *ptr);

    inline bool IsDeviceAddress(const void *buffer) {
        return IsPooledBuffer(buffer) || IsAdditionalDeviceAddress(buffer);
    }

}
