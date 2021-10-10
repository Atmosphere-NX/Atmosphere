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

namespace ams::spl::impl {

    class DeviceAddressMapper {
        private:
            dd::DeviceAddressSpaceType *m_das;
            u64 m_process_address;
            size_t m_size;
            dd::DeviceVirtualAddress m_device_address;
        public:
            DeviceAddressMapper(dd::DeviceAddressSpaceType *das, u64 process_address, size_t size, dd::DeviceVirtualAddress device_address, dd::MemoryPermission permission)
                : m_das(das), m_process_address(process_address), m_size(size), m_device_address(device_address)
            {
                R_ABORT_UNLESS(dd::MapDeviceAddressSpaceAligned(m_das, dd::GetCurrentProcessHandle(), m_process_address, m_size, m_device_address, permission));
            }

            ~DeviceAddressMapper() {
                dd::UnmapDeviceAddressSpace(m_das, dd::GetCurrentProcessHandle(), m_process_address, m_size, m_device_address);
            }
    };

}
