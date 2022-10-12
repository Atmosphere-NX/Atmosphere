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
#include "os_address_space_allocator.hpp"

#if defined(ATMOSPHERE_OS_HORIZON)
    #include "os_stack_guard_manager_impl.os.horizon.hpp"
#elif defined(ATMOSPHERE_OS_WINDOWS)
    #include "os_stack_guard_manager_impl.os.windows.hpp"
#elif defined(ATMOSPHERE_OS_LINUX)
    #include "os_stack_guard_manager_impl.os.linux.hpp"
#elif defined(ATMOSPHERE_OS_MACOS)
    #include "os_stack_guard_manager_impl.os.macos.hpp"
#else
    #error "Unknown OS for StackGuardManagerImpl"
#endif

namespace ams::os::impl {

    constexpr inline size_t StackGuardSize = 4 * os::MemoryPageSize;

    class StackGuardManager {
        private:
            StackGuardManagerImpl m_impl;
            AddressSpaceAllocator m_allocator;
        public:
            StackGuardManager() : m_impl(), m_allocator(m_impl.GetStackGuardBeginAddress(), m_impl.GetStackGuardEndAddress(), StackGuardSize, nullptr, 0) {
                /* ... */
            }

            void *AllocateStackGuardSpace(size_t size) {
                return reinterpret_cast<void *>(m_allocator.AllocateSpace(size, os::MemoryPageSize, 0, os::impl::AddressSpaceAllocatorDefaultGenerateRandom));
            }

            bool CheckGuardSpace(uintptr_t address, size_t size) {
                return m_allocator.CheckGuardSpace(address, size, StackGuardSize);
            }

            AddressAllocationResult MapAtRandomAddress(void **out, bool (*map)(const void *, const void *, size_t), void (*unmap)(const void *, const void *, size_t), const void *address, size_t size) {
                /* Try to map up to 0x40 times. */
                constexpr int TryCountMax = 0x40;
                for (auto i = 0; i < TryCountMax; ++i) {
                    /* Get stack guard space. */
                    void * const space = this->AllocateStackGuardSpace(size);
                    if (space == nullptr) {
                        return AddressAllocationResult_OutOfMemory;
                    }

                    /* Try to map. */
                    if (map(space, address, size)) {
                        /* Check that the guard space is still there. */
                        if (this->CheckGuardSpace(reinterpret_cast<uintptr_t>(space), size)) {
                            *out = space;
                            return AddressAllocationResult_Success;
                        } else {
                            /* We need to retry. */
                            unmap(space, address, size);
                        }
                    }
                }

                /* We failed. */
                return AddressAllocationResult_OutOfSpace;
            }
    };

}