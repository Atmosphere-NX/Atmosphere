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
#include <stratosphere/os/os_io_region_types.hpp>
#include <stratosphere/os/os_io_region_api.hpp>

namespace ams::os {

    class IoRegion {
        NON_COPYABLE(IoRegion);
        NON_MOVEABLE(IoRegion);
        private:
            IoRegionType m_io_region;
        public:
            constexpr IoRegion() : m_io_region{ .state = IoRegionType::State_NotInitialized } {
                /* ... */
            }

            IoRegion(NativeHandle io_pool_handle, uintptr_t address, size_t size, MemoryMapping mapping, MemoryPermission permission) {
                R_ABORT_UNLESS(CreateIoRegion(std::addressof(m_io_region), io_pool_handle, address, size, mapping, permission));
            }

            IoRegion(size_t size, NativeHandle handle, bool managed) {
                this->AttachHandle(size, handle, managed);
            }

            ~IoRegion() {
                if (m_io_region.state == IoRegionType::State_NotInitialized) {
                    return;
                }

                if (m_io_region.state == IoRegionType::State_Mapped) {
                    this->Unmap();
                }

                DestroyIoRegion(std::addressof(m_io_region));
            }

            void AttachHandle(size_t size, NativeHandle handle, bool managed) {
                AttachIoRegionHandle(std::addressof(m_io_region), size, handle, managed);
            }

            NativeHandle GetHandle() const {
                return GetIoRegionHandle(std::addressof(m_io_region));
            }

            Result Map(void **out, MemoryPermission perm) {
                return MapIoRegion(out, std::addressof(m_io_region), perm);
            }

            void Unmap() {
                UnmapIoRegion(std::addressof(m_io_region));
            }

            operator IoRegionType &() {
                return m_io_region;
            }

            operator const IoRegionType &() const {
                return m_io_region;
            }

            IoRegionType *GetBase() {
                return std::addressof(m_io_region);
            }
    };

}
