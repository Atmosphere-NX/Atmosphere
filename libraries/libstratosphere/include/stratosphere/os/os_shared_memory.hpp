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
#include <stratosphere/os/os_shared_memory_types.hpp>
#include <stratosphere/os/os_shared_memory_api.hpp>

namespace ams::os {

    class SharedMemory {
        NON_COPYABLE(SharedMemory);
        NON_MOVEABLE(SharedMemory);
        private:
            SharedMemoryType m_shared_memory;
        public:
            constexpr SharedMemory() : m_shared_memory{ .state = SharedMemoryType::State_NotInitialized } {
                /* ... */
            }

            SharedMemory(size_t size, MemoryPermission my_perm, MemoryPermission other_perm) {
                R_ABORT_UNLESS(CreateSharedMemory(std::addressof(m_shared_memory), size, my_perm, other_perm));
            }

            SharedMemory(size_t size, NativeHandle handle, bool managed) {
                this->Attach(size, handle, managed);
            }

            ~SharedMemory() {
                if (m_shared_memory.state == SharedMemoryType::State_NotInitialized) {
                    return;
                }
                DestroySharedMemory(std::addressof(m_shared_memory));
            }

            void Attach(size_t size, NativeHandle handle, bool managed) {
                return AttachSharedMemory(std::addressof(m_shared_memory), size, handle, managed);
            }

            void *Map(MemoryPermission perm) {
                return MapSharedMemory(std::addressof(m_shared_memory), perm);
            }

            void Unmap() {
                return UnmapSharedMemory(std::addressof(m_shared_memory));
            }

            void *GetMappedAddress() const {
                return GetSharedMemoryAddress(std::addressof(m_shared_memory));
            }

            size_t GetSize() const {
                return GetSharedMemorySize(std::addressof(m_shared_memory));
            }

            NativeHandle GetHandle() const {
                return GetSharedMemoryHandle(std::addressof(m_shared_memory));
            }

            operator SharedMemoryType &() {
                return m_shared_memory;
            }

            operator const SharedMemoryType &() const {
                return m_shared_memory;
            }

            SharedMemoryType *GetBase() {
                return std::addressof(m_shared_memory);
            }
    };

}
