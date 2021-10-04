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

namespace ams::map {

    namespace {

        /* Convenience defines. */
        constexpr size_t GuardRegionSize = 0x4000;
        constexpr size_t LocateRetryCount = 0x200;

        /* Deprecated/Modern implementations. */
        Result LocateMappableSpaceDeprecated(uintptr_t *out_address, size_t size) {
            svc::MemoryInfo mem_info;
            svc::PageInfo page_info;
            uintptr_t cur_base = 0;

            AddressSpaceInfo address_space;
            R_TRY(GetProcessAddressSpaceInfo(&address_space, dd::GetCurrentProcessHandle()));
            cur_base = address_space.aslr_base;

            do {
                R_TRY(svc::QueryMemory(&mem_info, &page_info, cur_base));

                if (mem_info.state == svc::MemoryState_Free && mem_info.addr - cur_base + mem_info.size >= size) {
                    *out_address = cur_base;
                    return ResultSuccess();
                }

                const uintptr_t mem_end = mem_info.addr + mem_info.size;
                R_UNLESS(mem_info.state != svc::MemoryState_Inaccessible,                    svc::ResultOutOfMemory());
                R_UNLESS(cur_base <= mem_end,                                                svc::ResultOutOfMemory());
                R_UNLESS(mem_end <= static_cast<uintptr_t>(std::numeric_limits<s32>::max()), svc::ResultOutOfMemory());

                cur_base = mem_end;
            } while (true);
        }

        Result LocateMappableSpaceModern(uintptr_t *out_address, size_t size) {
            svc::MemoryInfo mem_info;
            svc::PageInfo page_info;
            uintptr_t cur_base = 0, cur_end = 0;

            AddressSpaceInfo address_space;
            R_TRY(GetProcessAddressSpaceInfo(&address_space, dd::GetCurrentProcessHandle()));
            cur_base = address_space.aslr_base;
            cur_end = cur_base + size;

            R_UNLESS(cur_base < cur_end, svc::ResultOutOfMemory());

            while (true) {
                if (address_space.heap_size && (address_space.heap_base <= cur_end - 1 && cur_base <= address_space.heap_end - 1)) {
                    /* If we overlap the heap region, go to the end of the heap region. */
                    R_UNLESS(cur_base != address_space.heap_end, svc::ResultOutOfMemory());
                    cur_base = address_space.heap_end;
                } else if (address_space.alias_size && (address_space.alias_base <= cur_end - 1 && cur_base <= address_space.alias_end - 1)) {
                    /* If we overlap the alias region, go to the end of the alias region. */
                    R_UNLESS(cur_base != address_space.alias_end, svc::ResultOutOfMemory());
                    cur_base = address_space.alias_end;
                } else {
                    R_ABORT_UNLESS(svc::QueryMemory(&mem_info, &page_info, cur_base));
                    if (mem_info.state == svc::MemoryState_Free && mem_info.addr - cur_base + mem_info.size >= size) {
                        *out_address = cur_base;
                        return ResultSuccess();
                    }
                    R_UNLESS(cur_base < mem_info.addr + mem_info.size, svc::ResultOutOfMemory());

                    cur_base = mem_info.addr + mem_info.size;
                    R_UNLESS(cur_base < address_space.aslr_end,        svc::ResultOutOfMemory());
                }
                cur_end = cur_base + size;
                R_UNLESS(cur_base < cur_base + size, svc::ResultOutOfMemory());
            }
        }

        Result MapCodeMemoryInProcessDeprecated(MappedCodeMemory &out_mcm, os::NativeHandle process_handle, uintptr_t base_address, size_t size) {
            AddressSpaceInfo address_space;
            R_TRY(GetProcessAddressSpaceInfo(&address_space, process_handle));

            R_UNLESS(size <= address_space.aslr_size, ro::ResultOutOfAddressSpace());

            uintptr_t try_address;
            for (unsigned int i = 0; i < LocateRetryCount; i++) {
                try_address = address_space.aslr_base + (os::GenerateRandomU64(static_cast<u64>(address_space.aslr_size - size) / os::MemoryPageSize) * os::MemoryPageSize);

                MappedCodeMemory tmp_mcm(process_handle, try_address, base_address, size);
                R_TRY_CATCH(tmp_mcm.GetResult()) {
                    R_CATCH(svc::ResultInvalidCurrentMemory) { continue; }
                } R_END_TRY_CATCH;

                if (!CanAddGuardRegionsInProcess(process_handle, try_address, size)) {
                    continue;
                }

                /* We're done searching. */
                out_mcm = std::move(tmp_mcm);
                return ResultSuccess();
            }

            return ro::ResultOutOfAddressSpace();
        }

        Result MapCodeMemoryInProcessModern(MappedCodeMemory &out_mcm, os::NativeHandle process_handle, uintptr_t base_address, size_t size) {
            AddressSpaceInfo address_space;
            R_TRY(GetProcessAddressSpaceInfo(&address_space, process_handle));

            R_UNLESS(size <= address_space.aslr_size, ro::ResultOutOfAddressSpace());

            uintptr_t try_address;
            for (unsigned int i = 0; i < LocateRetryCount; i++) {
                while (true) {
                    try_address = address_space.aslr_base + (os::GenerateRandomU64(static_cast<u64>(address_space.aslr_size - size) / os::MemoryPageSize) * os::MemoryPageSize);
                    if (address_space.heap_size && (address_space.heap_base <= try_address + size - 1 && try_address <= address_space.heap_end - 1)) {
                        continue;
                    }
                    if (address_space.alias_size && (address_space.alias_base <= try_address + size - 1 && try_address <= address_space.alias_end - 1)) {
                        continue;
                    }
                    break;
                }

                MappedCodeMemory tmp_mcm(process_handle, try_address, base_address, size);
                R_TRY_CATCH(tmp_mcm.GetResult()) {
                    R_CATCH(svc::ResultInvalidCurrentMemory) { continue; }
                } R_END_TRY_CATCH;

                if (!CanAddGuardRegionsInProcess(process_handle, try_address, size)) {
                    continue;
                }

                /* We're done searching. */
                out_mcm = std::move(tmp_mcm);
                return ResultSuccess();
            }

            return ro::ResultOutOfAddressSpace();
        }

    }

    /* Public API. */
    Result GetProcessAddressSpaceInfo(AddressSpaceInfo *out, os::NativeHandle process_h) {
        /* Clear output. */
        std::memset(out, 0, sizeof(*out));

        /* Retrieve info from kernel. */
        R_TRY(svc::GetInfo(&out->heap_base,  svc::InfoType_HeapRegionAddress, process_h, 0));
        R_TRY(svc::GetInfo(&out->heap_size,  svc::InfoType_HeapRegionSize, process_h, 0));
        R_TRY(svc::GetInfo(&out->alias_base, svc::InfoType_AliasRegionAddress, process_h, 0));
        R_TRY(svc::GetInfo(&out->alias_size, svc::InfoType_AliasRegionSize, process_h, 0));
        R_TRY(svc::GetInfo(&out->aslr_base,  svc::InfoType_AslrRegionAddress, process_h, 0));
        R_TRY(svc::GetInfo(&out->aslr_size,  svc::InfoType_AslrRegionSize, process_h, 0));

        out->heap_end  = out->heap_base + out->heap_size;
        out->alias_end = out->alias_base + out->alias_size;
        out->aslr_end  = out->aslr_base + out->aslr_size;
        return ResultSuccess();
    }

    Result LocateMappableSpace(uintptr_t *out_address, size_t size) {
        if (hos::GetVersion() >= hos::Version_2_0_0) {
            return LocateMappableSpaceModern(out_address, size);
        } else {
            return LocateMappableSpaceDeprecated(out_address, size);
        }
    }

    Result MapCodeMemoryInProcess(MappedCodeMemory &out_mcm, os::NativeHandle process_handle, uintptr_t base_address, size_t size) {
        if (hos::GetVersion() >= hos::Version_2_0_0) {
            return MapCodeMemoryInProcessModern(out_mcm, process_handle, base_address, size);
        } else {
            return MapCodeMemoryInProcessDeprecated(out_mcm, process_handle, base_address, size);
        }
    }

    bool CanAddGuardRegionsInProcess(os::NativeHandle process_handle, uintptr_t address, size_t size) {
        svc::MemoryInfo mem_info;
        svc::PageInfo page_info;

        /* Nintendo doesn't validate SVC return values at all. */
        /* TODO: Should we allow these to fail? */
        R_ABORT_UNLESS(svc::QueryProcessMemory(&mem_info, &page_info, process_handle, address - 1));
        if (mem_info.state == svc::MemoryState_Free && address - GuardRegionSize >= mem_info.addr) {
            R_ABORT_UNLESS(svc::QueryProcessMemory(&mem_info, &page_info, process_handle, address + size));
            return mem_info.state == svc::MemoryState_Free && address + size + GuardRegionSize <= mem_info.addr + mem_info.size;
        }

        return false;
    }

}
