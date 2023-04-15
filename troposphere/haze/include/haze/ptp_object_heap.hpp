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

#include <haze/common.hpp>

namespace haze {

    /* This simple arena allocator implementation allows us to rapidly reclaim the entire object graph. */
    /* This is critical for maintaining interactivity when a session is closed. */
    class PtpObjectHeap {
        private:
            static constexpr size_t NumHeapBlocks = 2;

            void *m_heap_blocks[NumHeapBlocks];
            void *m_next_address;
            u32 m_heap_block_size;
            u32 m_current_heap_block;

        public:
            constexpr explicit PtpObjectHeap() : m_heap_blocks(), m_next_address(), m_heap_block_size(), m_current_heap_block() { /* ... */ }

            void Initialize();
            void Finalize();

        public:
            constexpr size_t GetSizeTotal() const {
                return m_heap_block_size * NumHeapBlocks;
            }

            constexpr size_t GetSizeUsed() const {
                return (m_heap_block_size * m_current_heap_block) + this->GetNextAddress() - this->GetFirstAddress();
            }

        private:
            constexpr u8 *GetNextAddress()  const { return static_cast<u8 *>(m_next_address); }
            constexpr u8 *GetFirstAddress() const { return static_cast<u8 *>(m_heap_blocks[m_current_heap_block]); }

            constexpr u8 *GetCurrentBlockEnd() const {
                return this->GetFirstAddress() + m_heap_block_size;
            }

            constexpr bool AllocationIsPossible(size_t n) const {
                return n <= m_heap_block_size;
            }

            constexpr bool AllocationIsSatisfyable(size_t n) const {
                if (this->GetNextAddress() + n < this->GetNextAddress()) return false;
                if (this->GetNextAddress() + n > this->GetCurrentBlockEnd()) return false;

                return true;
            }

            constexpr bool AdvanceToNextBlock() {
                if (m_current_heap_block < NumHeapBlocks - 1) {
                    m_next_address = m_heap_blocks[++m_current_heap_block];
                    return true;
                }

                return false;
            }

            constexpr void *AllocateFromCurrentBlock(size_t n) {
                void *result = this->GetNextAddress();

                m_next_address = this->GetNextAddress() + n;

                return result;
            }

        public:
            template <typename T = void>
            constexpr T *Allocate(size_t n) {
                if (n + 7 < n) return nullptr;

                /* Round up the amount to a multiple of 8. */
                n = (n + 7) & ~7ull;

                /* Check if the allocation is possible. */
                if (!this->AllocationIsPossible(n)) return nullptr;

                /* If the allocation is not satisfyable now, we might be able to satisfy it on the next block. */
                /* However, if the next block would be empty, we won't be able to satisfy the request. */
                if (!this->AllocationIsSatisfyable(n) && !this->AdvanceToNextBlock()) {
                    return nullptr;
                }

                /* Allocate the memory. */
                return static_cast<T *>(this->AllocateFromCurrentBlock(n));
            }

            constexpr void Deallocate(void *p, size_t n) {
                /* If the pointer was the last allocation, return the memory to the heap. */
                if (static_cast<u8 *>(p) + n == this->GetNextAddress()) {
                    m_next_address = this->GetNextAddress() - n;
                }

                /* Otherwise, do nothing. */
            }

            template <typename T, typename... Args>
            constexpr T *New(Args&&... args) {
                T *t = static_cast<T *>(this->Allocate(sizeof(T)));
                std::construct_at(t, std::forward<Args>(args)...);
                return t;
            }

            template <typename T>
            constexpr void Delete(T *t) {
                std::destroy_at(t);
                this->Deallocate(t, sizeof(T));
            }
    };

}
