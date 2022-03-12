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

namespace ams::fs {

    class IBufferManager {
        public:
            class BufferAttribute {
                private:
                    s32 m_level;
                public:
                    constexpr BufferAttribute() : m_level(0) { /* ... */ }
                    constexpr explicit BufferAttribute(s32 l) : m_level(l) { /* ... */ }

                    constexpr s32 GetLevel() const { return m_level; }
            };

            using CacheHandle = u64;

            static constexpr s32 BufferLevelMin = 0;

            using MemoryRange = std::pair<uintptr_t, size_t>;

            static constexpr ALWAYS_INLINE MemoryRange MakeMemoryRange(uintptr_t address, size_t size) { return MemoryRange(address, size); }
        public:
            virtual ~IBufferManager() { /* ... */ }

            ALWAYS_INLINE const MemoryRange AllocateBuffer(size_t size, const BufferAttribute &attr) {
                return this->DoAllocateBuffer(size, attr);
            }

            ALWAYS_INLINE const MemoryRange AllocateBuffer(size_t size) {
                return this->DoAllocateBuffer(size, BufferAttribute());
            }

            ALWAYS_INLINE void DeallocateBuffer(uintptr_t address, size_t size) {
                return this->DoDeallocateBuffer(address, size);
            }

            ALWAYS_INLINE void DeallocateBuffer(const MemoryRange &memory_range) {
                return this->DoDeallocateBuffer(memory_range.first, memory_range.second);
            }

            ALWAYS_INLINE CacheHandle RegisterCache(uintptr_t address, size_t size, const BufferAttribute &attr) {
                return this->DoRegisterCache(address, size, attr);
            }

            ALWAYS_INLINE CacheHandle RegisterCache(const MemoryRange &memory_range, const BufferAttribute &attr) {
                return this->DoRegisterCache(memory_range.first, memory_range.second, attr);
            }

            ALWAYS_INLINE const std::pair<uintptr_t, size_t> AcquireCache(CacheHandle handle) {
                return this->DoAcquireCache(handle);
            }

            ALWAYS_INLINE size_t GetTotalSize() const {
                return this->DoGetTotalSize();
            }

            ALWAYS_INLINE size_t GetFreeSize() const {
                return this->DoGetFreeSize();
            }

            ALWAYS_INLINE size_t GetTotalAllocatableSize() const {
                return this->DoGetTotalAllocatableSize();
            }

            ALWAYS_INLINE size_t GetFreeSizePeak() const {
                return this->DoGetFreeSizePeak();
            }

            ALWAYS_INLINE size_t GetTotalAllocatableSizePeak() const {
                return this->DoGetTotalAllocatableSizePeak();
            }

            ALWAYS_INLINE size_t GetRetriedCount() const {
                return this->DoGetRetriedCount();
            }

            ALWAYS_INLINE void ClearPeak() {
                return this->DoClearPeak();
            }
        protected:
            virtual const MemoryRange DoAllocateBuffer(size_t size, const BufferAttribute &attr) = 0;

            virtual void DoDeallocateBuffer(uintptr_t address, size_t size) = 0;

            virtual CacheHandle DoRegisterCache(uintptr_t address, size_t size, const BufferAttribute &attr) = 0;

            virtual const MemoryRange DoAcquireCache(CacheHandle handle) = 0;

            virtual size_t DoGetTotalSize() const = 0;

            virtual size_t DoGetFreeSize() const = 0;

            virtual size_t DoGetTotalAllocatableSize() const = 0;

            virtual size_t DoGetFreeSizePeak() const = 0;

            virtual size_t DoGetTotalAllocatableSizePeak() const = 0;

            virtual size_t DoGetRetriedCount() const = 0;

            virtual void DoClearPeak() = 0;
    };

}
