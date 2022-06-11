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
#include <stratosphere/windows.hpp>

namespace ams::os::impl {

    class MemoryHeapManagerWindowsImpl {
        NON_COPYABLE(MemoryHeapManagerWindowsImpl);
        NON_MOVEABLE(MemoryHeapManagerWindowsImpl);
        private:
            LPVOID m_real_reserved_address;
            size_t m_real_reserved_size;
            LPVOID m_aligned_reserved_heap_address;
            size_t m_aligned_reserved_heap_size;
            size_t m_committed_size;
        public:
            MemoryHeapManagerWindowsImpl() : m_real_reserved_address(nullptr), m_real_reserved_size(0), m_aligned_reserved_heap_address(nullptr), m_aligned_reserved_heap_size(0), m_committed_size(0) {
                /* Define target size. */
                constexpr size_t TargetReservedSize = 32_GB;

                /* Allocate appropriate amount of virtual space. */
                size_t reserved_size   = 0;
                size_t reserved_addend = TargetReservedSize;
                while (reserved_addend >= MemoryHeapUnitSize) {
                    if (this->ReserveVirtualSpace(0, reserved_size + reserved_addend)) {
                        this->ReleaseVirtualSpace();

                        reserved_size += reserved_addend;
                        if (reserved_size >= TargetReservedSize) {
                            break;
                        }
                    }

                    reserved_addend /= 2;
                }

                /* Reserve virtual space. */
                AMS_ABORT_UNLESS(this->ReserveVirtualSpace(0, reserved_size));
            }

            Result SetHeapSize(uintptr_t *out, size_t size) {
                /* Check that we have a reserved address. */
                R_UNLESS(m_real_reserved_address != nullptr, os::ResultOutOfMemory());

                /* If necessary, commit the new memory. */
                if (size > m_committed_size) {
                    R_UNLESS(this->CommitMemory(size), os::ResultOutOfMemory());
                } else if (size < m_committed_size) {
                    /* Otherwise, decommit. */
                    this->DecommitMemory(reinterpret_cast<uintptr_t>(m_aligned_reserved_heap_address) + size, m_committed_size - size);
                }

                /* Set the committed size. */
                m_committed_size = size;

                /* Set the out address. */
                *out = reinterpret_cast<uintptr_t>(m_aligned_reserved_heap_address);
                R_SUCCEED();
            }
        private:
            bool ReserveVirtualSpace(uintptr_t address, size_t size) {
                AMS_ABORT_UNLESS(m_real_reserved_address == nullptr);
                AMS_ABORT_UNLESS(m_real_reserved_size == 0);

                size_t reserve_size = util::AlignUp(size, MemoryHeapUnitSize);
                if constexpr (constexpr size_t VirtualAllocUnitSize = 64_KB; MemoryHeapUnitSize > VirtualAllocUnitSize) {
                    reserve_size += MemoryHeapUnitSize - VirtualAllocUnitSize;
                }

                LPVOID res = ::VirtualAlloc(reinterpret_cast<LPVOID>(address), reserve_size, MEM_RESERVE, PAGE_READWRITE);
                if (res == nullptr) {
                    return false;
                }

                m_real_reserved_address = res;
                m_real_reserved_size    = reserve_size;

                m_aligned_reserved_heap_address = reinterpret_cast<LPVOID>(util::AlignUp(reinterpret_cast<uintptr_t>(m_real_reserved_address), MemoryHeapUnitSize));
                m_aligned_reserved_heap_size    = size;

                return true;
            }

            void ReleaseVirtualSpace() {
                if (m_real_reserved_address != nullptr) {
                    auto res = ::VirtualFree(m_real_reserved_address, 0, MEM_RELEASE);
                    AMS_ASSERT(res);
                    AMS_UNUSED(res);

                    m_real_reserved_address = nullptr;
                    m_real_reserved_size    = 0;

                    m_aligned_reserved_heap_address = nullptr;
                    m_aligned_reserved_heap_size    = 0;
                }
            }

            bool CommitMemory(size_t size) {
                LPVOID address = ::VirtualAlloc(m_aligned_reserved_heap_address, static_cast<SIZE_T>(size), MEM_COMMIT, PAGE_READWRITE);
                if (address == nullptr) {
                    return false;
                }

                AMS_ABORT_UNLESS(address == m_aligned_reserved_heap_address);
                return true;
            }

            void DecommitMemory(uintptr_t address, size_t size) {
                auto res = ::VirtualFree(reinterpret_cast<LPVOID>(address), static_cast<SIZE_T>(size), MEM_DECOMMIT);
                AMS_ASSERT(res);
                AMS_UNUSED(res);
            }
    };

    using MemoryHeapManagerImpl = MemoryHeapManagerWindowsImpl;

}
