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
#include <sys/mman.h>

namespace ams::os::impl {

    void SetMemoryPermissionImpl(uintptr_t address, size_t size, MemoryPermission perm) {
        switch (perm) {
            case MemoryPermission_None:
                {
                    auto res = ::mprotect(reinterpret_cast<void *>(address), size, PROT_NONE);
                    AMS_ABORT_UNLESS(res);
                    AMS_UNUSED(res);
                }
                break;
            case MemoryPermission_ReadOnly:
                {
                    auto res = ::mprotect(reinterpret_cast<void *>(address), size, PROT_READ);
                    AMS_ABORT_UNLESS(res);
                    AMS_UNUSED(res);
                }
                break;
            case MemoryPermission_ReadWrite:
                {
                    auto res = ::mprotect(reinterpret_cast<void *>(address), size, PROT_READ | PROT_WRITE);
                    AMS_ABORT_UNLESS(res);
                    AMS_UNUSED(res);
                }
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

}
