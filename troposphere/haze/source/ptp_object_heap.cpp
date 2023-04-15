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
#include <haze.hpp>

namespace haze {

    namespace {

        /* Allow 20MiB for use by libnx. */
        static constexpr size_t LibnxReservedMem = 20_MB;

    }

    void PtpObjectHeap::Initialize() {
        size_t mem_used = 0;

        /* Skip re-initialization if we are currently initialized. */
        if (m_heap_block_size != 0) return;

        /* Estimate how much memory we can reserve. */
        HAZE_R_ABORT_UNLESS(svcGetInfo(std::addressof(mem_used), InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0));
        HAZE_ASSERT(mem_used > LibnxReservedMem);
        mem_used -= LibnxReservedMem;

        /* Take the rest for ourselves. */
        m_heap_block_size = mem_used / NumHeapBlocks;
        HAZE_ASSERT(m_heap_block_size > 0);

        /* Allocate the memory. */
        for (size_t i = 0; i < NumHeapBlocks; i++) {
            m_heap_blocks[i] = std::malloc(m_heap_block_size);
            HAZE_ASSERT(m_heap_blocks[i] != nullptr);
        }

        /* Set the address to allocate from. */
        m_next_address = m_heap_blocks[0];
    }

    void PtpObjectHeap::Finalize() {
        if (m_heap_block_size == 0) return;

        /* Tear down the heap, allowing a subsequent call to Initialize() if desired. */
        for (size_t i = 0; i < NumHeapBlocks; i++) {
            std::free(m_heap_blocks[i]);
            m_heap_blocks[i] = nullptr;
        }

        m_next_address       = nullptr;
        m_heap_block_size    = 0;
        m_current_heap_block = 0;
    }

}
