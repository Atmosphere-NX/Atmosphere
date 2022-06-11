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
#include <stratosphere.hpp>
#include <sys/mman.h>

namespace ams::os::impl {

    class MemoryHeapManagerMacosImpl {
        NON_COPYABLE(MemoryHeapManagerMacosImpl);
        NON_MOVEABLE(MemoryHeapManagerMacosImpl);
        private:
            uintptr_t m_real_reserved_address;
            size_t m_real_reserved_size;
            uintptr_t m_aligned_reserved_heap_address;
            size_t m_aligned_reserved_heap_size;
            size_t m_committed_size;
        public:
            MemoryHeapManagerMacosImpl() : m_real_reserved_address(0), m_real_reserved_size(0), m_aligned_reserved_heap_address(0), m_aligned_reserved_heap_size(0), m_committed_size(0) {
                /* Reserve a 32 GB region of virtual address space. */
                constexpr size_t TargetReservedSize = 32_GB;
                const auto reserved = ::mmap(nullptr, TargetReservedSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                AMS_ABORT_UNLESS(reserved != MAP_FAILED);

                m_real_reserved_address = reinterpret_cast<uintptr_t>(reserved);
                m_real_reserved_size    = TargetReservedSize;

                m_aligned_reserved_heap_address = util::AlignUp(m_real_reserved_address, MemoryHeapUnitSize);
                m_aligned_reserved_heap_size    = m_real_reserved_size - MemoryHeapUnitSize;
            }

            Result SetHeapSize(uintptr_t *out, size_t size) {
                /* Check that we have a reserved address. */
                R_UNLESS(m_real_reserved_address != 0, os::ResultOutOfMemory());

                /* If necessary, commit the new memory. */
                if (size > m_committed_size) {
                    R_UNLESS(this->CommitMemory(size), os::ResultOutOfMemory());
                } else if (size < m_committed_size) {
                    /* Otherwise, decommit. */
                    this->DecommitMemory(m_aligned_reserved_heap_address + size, m_committed_size - size);
                }

                /* Set the committed size. */
                m_committed_size = size;

                /* Set the out address. */
                *out = m_aligned_reserved_heap_address;
                R_SUCCEED();
            }
        private:
            bool CommitMemory(size_t size) {
                const auto res = ::mprotect(reinterpret_cast<void *>(m_aligned_reserved_heap_address), size, PROT_READ | PROT_WRITE);
                return res == 0;
            }

            void DecommitMemory(uintptr_t address, size_t size) {
                const auto reserved = ::mmap(reinterpret_cast<void *>(address), size, PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                AMS_ABORT_UNLESS(reserved != MAP_FAILED);
            }
    };

    using MemoryHeapManagerImpl = MemoryHeapManagerMacosImpl;

}
