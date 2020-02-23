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
#include <mesosphere.hpp>

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {



    }

    /* =============================    64 ABI    ============================= */

    Result MapSharedMemory64(ams::svc::Handle shmem_handle, ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission map_perm) {
        MESOSPHERE_PANIC("Stubbed SvcMapSharedMemory64 was called.");
    }

    Result UnmapSharedMemory64(ams::svc::Handle shmem_handle, ams::svc::Address address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcUnmapSharedMemory64 was called.");
    }

    Result CreateSharedMemory64(ams::svc::Handle *out_handle, ams::svc::Size size, ams::svc::MemoryPermission owner_perm, ams::svc::MemoryPermission remote_perm) {
        MESOSPHERE_PANIC("Stubbed SvcCreateSharedMemory64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    Result MapSharedMemory64From32(ams::svc::Handle shmem_handle, ams::svc::Address address, ams::svc::Size size, ams::svc::MemoryPermission map_perm) {
        MESOSPHERE_PANIC("Stubbed SvcMapSharedMemory64From32 was called.");
    }

    Result UnmapSharedMemory64From32(ams::svc::Handle shmem_handle, ams::svc::Address address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcUnmapSharedMemory64From32 was called.");
    }

    Result CreateSharedMemory64From32(ams::svc::Handle *out_handle, ams::svc::Size size, ams::svc::MemoryPermission owner_perm, ams::svc::MemoryPermission remote_perm) {
        MESOSPHERE_PANIC("Stubbed SvcCreateSharedMemory64From32 was called.");
    }

}
