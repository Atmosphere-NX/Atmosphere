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
#include <stratosphere.hpp>
#include <stratosphere/windows.hpp>

namespace ams::os::impl {

    void SetMemoryPermissionImpl(uintptr_t address, size_t size, MemoryPermission perm) {
        DWORD old;

        uintptr_t cur_address = address;
        size_t remaining = size;
        while (remaining > 0) {
            const size_t cur_size = std::min<size_t>(remaining, 2_GB);
            switch (perm) {
                case MemoryPermission_None:
                    {
                        auto res = ::VirtualProtect(reinterpret_cast<LPVOID>(cur_address), static_cast<DWORD>(cur_size), PAGE_NOACCESS, std::addressof(old));
                        AMS_ABORT_UNLESS(res);
                        AMS_UNUSED(res);
                    }
                    break;
                case MemoryPermission_ReadOnly:
                    {
                        auto res = ::VirtualProtect(reinterpret_cast<LPVOID>(cur_address), static_cast<DWORD>(cur_size), PAGE_READONLY, std::addressof(old));
                        AMS_ABORT_UNLESS(res);
                        AMS_UNUSED(res);
                    }
                    break;
                case MemoryPermission_ReadWrite:
                    {
                        auto res = ::VirtualProtect(reinterpret_cast<LPVOID>(cur_address), static_cast<DWORD>(cur_size), PAGE_READWRITE, std::addressof(old));
                        AMS_ABORT_UNLESS(res);
                        AMS_UNUSED(res);
                    }
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            cur_address += cur_size;
            remaining   -= cur_size;
        }
    }

}
