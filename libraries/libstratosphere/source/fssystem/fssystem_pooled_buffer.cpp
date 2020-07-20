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
#include <stratosphere.hpp>

namespace ams::fssystem {

    namespace {

        class AdditionalDeviceAddressEntry {
            private:
                /* TODO: SdkMutex */
                os::Mutex mutex;
                bool is_registered;
                uintptr_t address;
                size_t size;
            public:
                constexpr AdditionalDeviceAddressEntry() : mutex(false), is_registered(), address(), size() { /* ... */ }

                void Register(uintptr_t addr, size_t sz) {
                    std::scoped_lock lk(this->mutex);

                    AMS_ASSERT(!this->is_registered);
                    if (!this->is_registered) {
                        this->is_registered = true;
                        this->address       = addr;
                        this->size          = size;
                    }
                }

                void Unregister(uintptr_t addr) {
                    std::scoped_lock lk(this->mutex);

                    if (this->is_registered && this->address == addr) {
                        this->is_registered = false;
                        this->address       = 0;
                        this->size          = 0;
                    }
                }

                bool Includes(const void *ptr) {
                    std::scoped_lock lk(this->mutex);

                    if (this->is_registered) {
                        const uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
                        return this->address <= addr && addr < this->address + this->size;
                    } else {
                        return false;
                    }
                }
        };

        constexpr auto RetryWait = TimeSpan::FromMilliSeconds(10);

        constexpr size_t HeapBlockSize = BufferPoolAlignment;
        static_assert(HeapBlockSize == 4_KB);

        /* A heap block is 4KB. An order is a power of two. */
        /* This gives blocks of the order 32KB, 512KB, 4MB. */
        constexpr s32    HeapOrderTrim        = 3;
        constexpr s32    HeapOrderMax         = 7;
        constexpr s32    HeapOrderMaxForLarge = HeapOrderMax + 3;

        constexpr size_t HeapAllocatableSizeTrim        = HeapBlockSize * (static_cast<size_t>(1) << HeapOrderTrim);
        constexpr size_t HeapAllocatableSizeMax         = HeapBlockSize * (static_cast<size_t>(1) << HeapOrderMax);
        constexpr size_t HeapAllocatableSizeMaxForLarge = HeapBlockSize * (static_cast<size_t>(1) << HeapOrderMaxForLarge);

        /* TODO: SdkMutex */
        os::Mutex g_heap_mutex(false);
        FileSystemBuddyHeap g_heap;

        std::atomic<size_t> g_retry_count;
        std::atomic<size_t> g_reduce_allocation_count;

        void *g_heap_buffer;
        size_t g_heap_size;
        size_t g_heap_free_size_peak;

        AdditionalDeviceAddressEntry g_additional_device_address_entry;

    }

    size_t PooledBuffer::GetAllocatableSizeMaxCore(bool large) {
        return large ? HeapAllocatableSizeMaxForLarge : HeapAllocatableSizeMax;
    }

    void PooledBuffer::AllocateCore(size_t ideal_size, size_t required_size, bool large) {
        /* Ensure preconditions. */
        AMS_ASSERT(g_heap_buffer != nullptr);
        AMS_ASSERT(this->buffer == nullptr);
        AMS_ASSERT(g_heap.GetBlockSize() == HeapBlockSize);

        /* Check that we can allocate this size. */
        AMS_ASSERT(required_size <= GetAllocatableSizeMaxCore(large));

        const size_t target_size = std::min(std::max(ideal_size, required_size), GetAllocatableSizeMaxCore(large));

        /* Loop until we allocate. */
        while (true) {
            /* Lock the heap and try to allocate. */
            {
                std::scoped_lock lk(g_heap_mutex);

                /* Determine how much we can allocate, and don't allocate more than half the heap. */
                size_t allocatable_size = g_heap.GetAllocatableSizeMax();
                if (allocatable_size > HeapBlockSize) {
                    allocatable_size >>= 1;
                }

                /* Check if this allocation is acceptable. */
                if (allocatable_size >= required_size) {
                    /* Get the order. */
                    const auto order = g_heap.GetOrderFromBytes(std::min(target_size, allocatable_size));

                    /* Allocate and get the size. */
                    this->buffer = reinterpret_cast<char *>(g_heap.AllocateByOrder(order));
                    this->size   = g_heap.GetBytesFromOrder(order);
                }
            }

            /* Check if we allocated. */
            if (this->buffer != nullptr) {
                /* If we need to trim the end, do so. */
                if (this->GetSize() >= target_size + HeapAllocatableSizeTrim) {
                    this->Shrink(util::AlignUp(target_size, HeapAllocatableSizeTrim));
                }
                AMS_ASSERT(this->GetSize() >= required_size);

                /* If we reduced, note so. */
                if (this->GetSize() < std::min(target_size, HeapAllocatableSizeMax)) {
                    g_reduce_allocation_count++;
                }
                break;
            } else {
                /* Sleep. */
                os::SleepThread(RetryWait);
                g_retry_count++;
            }
        }

        /* Update metrics. */
        {
            std::scoped_lock lk(g_heap_mutex);

            const size_t free_size = g_heap.GetTotalFreeSize();
            if (free_size < g_heap_free_size_peak) {
                g_heap_free_size_peak = free_size;
            }
        }
    }

    void PooledBuffer::Shrink(size_t ideal_size) {
        AMS_ASSERT(ideal_size <= GetAllocatableSizeMaxCore(true));

        /* Check if we actually need to shrink. */
        if (this->size > ideal_size) {
            /* If we do, we need to have a buffer allocated from the heap. */
            AMS_ASSERT(this->buffer != nullptr);
            AMS_ASSERT(g_heap.GetBlockSize() == HeapBlockSize);

            const size_t new_size = util::AlignUp(ideal_size, HeapBlockSize);

            /* Repeatedly free the tail of our buffer until we're done. */
            {
                std::scoped_lock lk(g_heap_mutex);

                while (new_size < this->size) {
                    /* Determine the size and order to free. */
                    const size_t tail_align = util::LeastSignificantOneBit(this->size);
                    const size_t free_size  = std::min(util::FloorPowerOfTwo(this->size - new_size), tail_align);
                    const s32 free_order    = g_heap.GetOrderFromBytes(free_size);

                    /* Ensure we determined size correctly. */
                    AMS_ASSERT(util::IsAligned(free_size, HeapBlockSize));
                    AMS_ASSERT(free_size == g_heap.GetBytesFromOrder(free_order));

                    /* Actually free the memory. */
                    g_heap.Free(this->buffer + this->size - free_size, free_order);
                    this->size -= free_size;
                }
            }

            /* Shrinking to zero means that we have no buffer. */
            if (this->size == 0) {
                this->buffer = nullptr;
            }
        }
    }

    Result InitializeBufferPool(char *buffer, size_t size) {
        AMS_ASSERT(g_heap_buffer == nullptr);
        AMS_ASSERT(buffer != nullptr);
        AMS_ASSERT(util::IsAligned(reinterpret_cast<uintptr_t>(buffer), BufferPoolAlignment));

        /* Initialize the heap. */
        R_TRY(g_heap.Initialize(reinterpret_cast<uintptr_t>(buffer), size, HeapBlockSize, HeapOrderMaxForLarge + 1));

        /* Initialize metrics. */
        g_heap_buffer         = buffer;
        g_heap_size           = size;
        g_heap_free_size_peak = size;

        return ResultSuccess();
    }

    Result InitializeBufferPool(char *buffer, size_t size, char *work, size_t work_size) {
        AMS_ASSERT(g_heap_buffer == nullptr);
        AMS_ASSERT(buffer != nullptr);
        AMS_ASSERT(util::IsAligned(reinterpret_cast<uintptr_t>(buffer), BufferPoolAlignment));
        AMS_ASSERT(work_size >= BufferPoolWorkSize);

        /* Initialize the heap. */
        R_TRY(g_heap.Initialize(reinterpret_cast<uintptr_t>(buffer), size, HeapBlockSize, HeapOrderMaxForLarge + 1, work, work_size));

        /* Initialize metrics. */
        g_heap_buffer         = buffer;
        g_heap_size           = size;
        g_heap_free_size_peak = size;

        return ResultSuccess();
    }

    bool IsPooledBuffer(const void *buffer) {
        AMS_ASSERT(buffer != nullptr);
        return g_heap_buffer <= buffer && buffer < reinterpret_cast<char *>(g_heap_buffer) + g_heap_size;
    }

    size_t GetPooledBufferRetriedCount() {
        return g_retry_count;
    }

    size_t GetPooledBufferReduceAllocationCount() {
        return g_reduce_allocation_count;
    }

    size_t GetPooledBufferFreeSizePeak() {
        return g_heap_free_size_peak;
    }

    void ClearPooledBufferPeak() {
        std::scoped_lock lk(g_heap_mutex);
        g_heap_free_size_peak     = g_heap.GetTotalFreeSize();
        g_retry_count             = 0;
        g_reduce_allocation_count = 0;
    }

    void RegisterAdditionalDeviceAddress(uintptr_t address, size_t size) {
        g_additional_device_address_entry.Register(address, size);
    }

    void UnregisterAdditionalDeviceAddress(uintptr_t address) {
        g_additional_device_address_entry.Unregister(address);
    }

    bool IsAdditionalDeviceAddress(const void *ptr) {
        return g_additional_device_address_entry.Includes(ptr);
    }

}
