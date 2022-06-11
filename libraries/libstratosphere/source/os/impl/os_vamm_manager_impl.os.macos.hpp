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

    class VammManagerMacosImpl {
        public:
            static void GetReservedRegionImpl(uintptr_t *out_start, uintptr_t *out_size) {
                /* Reserve a 64 GB region of virtual address space. */
                constexpr size_t ReservedRegionSize = 64_GB;
                const auto reserved = ::mmap(nullptr, ReservedRegionSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                AMS_ABORT_UNLESS(reserved != MAP_FAILED);

                *out_start = reinterpret_cast<uintptr_t>(reserved);
                *out_size  = ReservedRegionSize;
            }

            static Result AllocatePhysicalMemoryImpl(uintptr_t address, size_t size) {
                R_UNLESS(::mprotect(reinterpret_cast<void *>(address), size, PROT_READ | PROT_WRITE) == 0, os::ResultOutOfMemory());
                R_SUCCEED();
            }

            static Result FreePhysicalMemoryImpl(uintptr_t address, size_t size) {
                const auto reserved = ::mmap(reinterpret_cast<void *>(address), size, PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                R_UNLESS(reserved != MAP_FAILED, os::ResultBusy());
                R_SUCCEED();
            }

            static consteval size_t GetExtraSystemResourceAssignedSize() {
                return 0;
            }

            static consteval size_t GetExtraSystemResourceUsedSize() {
                return 0;
            }

            static consteval bool IsVirtualAddressMemoryEnabled() {
                return true;
            }
    };

    using VammManagerImpl = VammManagerMacosImpl;

}
