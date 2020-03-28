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
#include <stratosphere.hpp>

namespace ams::os::impl {

    namespace {

        void SetMemoryPermissionBySvc(uintptr_t start, size_t size, svc::MemoryPermission perm) {
            uintptr_t cur_address = start;
            size_t remaining_size = size;
            while (remaining_size > 0) {
                svc::MemoryInfo mem_info;
                svc::PageInfo page_info;
                R_ABORT_UNLESS(svc::QueryMemory(std::addressof(mem_info), std::addressof(page_info), cur_address));

                size_t cur_size = std::min(mem_info.addr + mem_info.size - cur_address, remaining_size);

                if (mem_info.perm != perm) {
                    R_ABORT_UNLESS(svc::SetMemoryPermission(cur_address, cur_size, perm));
                }

                cur_address += cur_size;
                remaining_size -= cur_size;
            }
        }

    }

    void SetMemoryPermissionImpl(uintptr_t address, size_t size, MemoryPermission perm) {
        switch (perm) {
            case MemoryPermission_None:
                return SetMemoryPermissionBySvc(address, size, svc::MemoryPermission_None);
            case MemoryPermission_ReadOnly:
                return SetMemoryPermissionBySvc(address, size, svc::MemoryPermission_Read);
            case MemoryPermission_ReadWrite:
                return SetMemoryPermissionBySvc(address, size, svc::MemoryPermission_ReadWrite);
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

}
