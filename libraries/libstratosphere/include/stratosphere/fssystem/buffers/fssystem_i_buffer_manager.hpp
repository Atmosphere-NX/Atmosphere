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

    class IBufferManager {
        public:
            class BufferAttribute {
                private:
                    s32 level;
                public:
                    constexpr BufferAttribute() : level(0) { /* ... */ }
                    constexpr explicit BufferAttribute(s32 l) : level(l) { /* ... */ }

                    constexpr s32 GetLevel() const { return this->level; }
            };

            using CacheHandle = s64;

            static constexpr s32 BufferLevelMin = 0;
        public:
            virtual ~IBufferManager() { /* ... */ }

            const std::pair<uintptr_t, size_t> AllocateBuffer(size_t size, const BufferAttribute &attr) {
                return this->AllocateBufferImpl(size, attr);
            }

            const std::pair<uintptr_t, size_t> AllocateBuffer(size_t size) {
                return this->AllocateBufferImpl(size, BufferAttribute());
            }

            void DeallocateBuffer(uintptr_t address, size_t size) {
                return this->DeallocateBufferImpl(address, size);
            }

            CacheHandle RegisterCache(uintptr_t address, size_t size, const BufferAttribute &attr) {
                return this->RegisterCacheImpl(address, size, attr);
            }

            const std::pair<uintptr_t, size_t> AcquireCache(CacheHandle handle) {
                return this->AcquireCacheImpl(handle);
            }

            size_t GetTotalSize() const {
                return this->GetTotalSizeImpl();
            }

            size_t GetFreeSize() const {
                return this->GetFreeSizeImpl();
            }

            size_t GetTotalAllocatableSize() const {
                return this->GetTotalAllocatableSizeImpl();
            }

            size_t GetPeakFreeSize() const {
                return this->GetPeakFreeSizeImpl();
            }

            size_t GetPeakTotalAllocatableSize() const {
                return this->GetPeakTotalAllocatableSizeImpl();
            }

            size_t GetRetriedCount() const {
                return this->GetRetriedCountImpl();
            }

            void ClearPeak() const {
                return this->ClearPeak();
            }
        protected:
            virtual const std::pair<uintptr_t, size_t> AllocateBufferImpl(size_t size, const BufferAttribute &attr) = 0;

            virtual void DeallocateBufferImpl(uintptr_t address, size_t size) = 0;

            virtual CacheHandle RegisterCacheImpl(uintptr_t address, size_t size, const BufferAttribute &attr) = 0;

            virtual const std::pair<uintptr_t, size_t> AcquireCacheImpl(CacheHandle handle) = 0;

            virtual size_t GetTotalSizeImpl() const = 0;

            virtual size_t GetFreeSizeImpl() const = 0;

            virtual size_t GetTotalAllocatableSizeImpl() const = 0;

            virtual size_t GetPeakFreeSizeImpl() const = 0;

            virtual size_t GetPeakTotalAllocatableSizeImpl() const = 0;

            virtual size_t GetRetriedCountImpl() const = 0;

            virtual void ClearPeakImpl() = 0;
    };

}
