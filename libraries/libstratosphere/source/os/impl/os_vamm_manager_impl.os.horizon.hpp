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

namespace ams::os::impl {

    class VammManagerHorizonImpl {
        public:
            static void GetReservedRegionImpl(uintptr_t *out_start, uintptr_t *out_size) {
                u64 start, size, extra_size;
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(start),      svc::InfoType_AliasRegionAddress,   svc::PseudoHandle::CurrentProcess, 0));
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(size),       svc::InfoType_AliasRegionSize,      svc::PseudoHandle::CurrentProcess, 0));
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(extra_size), svc::InfoType_AliasRegionExtraSize, svc::PseudoHandle::CurrentProcess, 0));
                *out_start = start;
                *out_size  = size - extra_size;
            }

            static Result AllocatePhysicalMemoryImpl(uintptr_t address, size_t size) {
                R_TRY_CATCH(svc::MapPhysicalMemory(address, size)) {
                    R_CONVERT(svc::ResultOutOfResource, os::ResultOutOfResource())
                    R_CONVERT(svc::ResultOutOfMemory,   os::ResultOutOfMemory())
                    R_CONVERT(svc::ResultLimitReached,  os::ResultOutOfMemory())
                } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
                R_SUCCEED();
            }

            static Result FreePhysicalMemoryImpl(uintptr_t address, size_t size) {
                R_TRY_CATCH(svc::UnmapPhysicalMemory(address, size)) {
                    R_CONVERT(svc::ResultInvalidCurrentMemory, os::ResultBusy())
                    R_CONVERT(svc::ResultOutOfResource,        os::ResultOutOfResource())
                    R_CONVERT(svc::ResultOutOfMemory,          os::ResultOutOfMemory())
                    R_CONVERT(svc::ResultLimitReached,         os::ResultOutOfMemory())
                } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
                R_SUCCEED();
            }

            static size_t GetExtraSystemResourceAssignedSize() {
                u64 v;
                return R_SUCCEEDED(svc::GetInfo(std::addressof(v), svc::InfoType_SystemResourceSizeTotal, svc::PseudoHandle::CurrentProcess, 0)) ? v : 0;
            }

            static size_t GetExtraSystemResourceUsedSize() {
                u64 v;
                return R_SUCCEEDED(svc::GetInfo(std::addressof(v), svc::InfoType_SystemResourceSizeUsed, svc::PseudoHandle::CurrentProcess, 0)) ? v : 0;
            }

            static bool IsVirtualAddressMemoryEnabled() {
                return GetExtraSystemResourceAssignedSize() > 0;
            }
    };

    using VammManagerImpl = VammManagerHorizonImpl;

}
