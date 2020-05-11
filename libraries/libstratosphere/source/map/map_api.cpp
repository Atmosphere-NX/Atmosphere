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

namespace ams::map {

    namespace {

        /* Convenience defines. */
        constexpr size_t GuardRegionSize = 0x4000;
        constexpr size_t LocateRetryCount = 0x200;

        /* Deprecated/Modern implementations. */
        Result LocateMappableSpaceDeprecated(uintptr_t *out_address, size_t size) {
            MemoryInfo mem_info = {};
            u32 page_info = 0;
            uintptr_t cur_base = 0;

            AddressSpaceInfo address_space;
            R_TRY(GetProcessAddressSpaceInfo(&address_space, dd::GetCurrentProcessHandle()));
            cur_base = address_space.aslr_base;

            do {
                R_TRY(svcQueryMemory(&mem_info, &page_info, cur_base));

                if (mem_info.type == MemType_Unmapped && mem_info.addr - cur_base + mem_info.size >= size) {
                    *out_address = cur_base;
                    return ResultSuccess();
                }

                const uintptr_t mem_end = mem_info.addr + mem_info.size;
                R_UNLESS(mem_info.type != MemType_Reserved,                                  svc::ResultOutOfMemory());
                R_UNLESS(cur_base <= mem_end,                                                svc::ResultOutOfMemory());
                R_UNLESS(mem_end <= static_cast<uintptr_t>(std::numeric_limits<s32>::max()), svc::ResultOutOfMemory());

                cur_base = mem_end;
            } while (true);
        }

        Result LocateMappableSpaceModern(uintptr_t *out_address, size_t size) {
            MemoryInfo mem_info = {};
            u32 page_info = 0;
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
                    R_ABORT_UNLESS(svcQueryMemory(&mem_info, &page_info, cur_base));
                    if (mem_info.type == 0 && mem_info.addr - cur_base + mem_info.size >= size) {
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

        Result MapCodeMemoryInProcessDeprecated(MappedCodeMemory &out_mcm, Handle process_handle, uintptr_t base_address, size_t size) {
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

        Result MapCodeMemoryInProcessModern(MappedCodeMemory &out_mcm, Handle process_handle, uintptr_t base_address, size_t size) {
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
    Result GetProcessAddressSpaceInfo(AddressSpaceInfo *out, Handle process_h) {
        /* Clear output. */
        std::memset(out, 0, sizeof(*out));

        /* Retrieve info from kernel. */
        R_TRY(svcGetInfo(&out->heap_base,  InfoType_HeapRegionAddress, process_h, 0));
        R_TRY(svcGetInfo(&out->heap_size,  InfoType_HeapRegionSize, process_h, 0));
        R_TRY(svcGetInfo(&out->alias_base, InfoType_AliasRegionAddress, process_h, 0));
        R_TRY(svcGetInfo(&out->alias_size, InfoType_AliasRegionSize, process_h, 0));
        if (hos::GetVersion() >= hos::Version_2_0_0) {
            R_TRY(svcGetInfo(&out->aslr_base, InfoType_AslrRegionAddress, process_h, 0));
            R_TRY(svcGetInfo(&out->aslr_size, InfoType_AslrRegionSize, process_h, 0));
        } else {
            /* Auto-detect 32-bit vs 64-bit. */
            if (out->heap_base < AslrBase64BitDeprecated || out->alias_base < AslrBase64BitDeprecated) {
                out->aslr_base = AslrBase32Bit;
                out->aslr_size = AslrSize32Bit;
            } else {
                out->aslr_base = AslrBase64BitDeprecated;
                out->aslr_size = AslrSize64BitDeprecated;
            }
        }

        out->heap_end = out->heap_base + out->heap_size;
        out->alias_end = out->alias_base + out->alias_size;
        out->aslr_end = out->aslr_base + out->aslr_size;
        return ResultSuccess();
    }

    Result LocateMappableSpace(uintptr_t *out_address, size_t size) {
        if (hos::GetVersion() >= hos::Version_2_0_0) {
            return LocateMappableSpaceModern(out_address, size);
        } else {
            return LocateMappableSpaceDeprecated(out_address, size);
        }
    }

    Result MapCodeMemoryInProcess(MappedCodeMemory &out_mcm, Handle process_handle, uintptr_t base_address, size_t size) {
        if (hos::GetVersion() >= hos::Version_2_0_0) {
            return MapCodeMemoryInProcessModern(out_mcm, process_handle, base_address, size);
        } else {
            return MapCodeMemoryInProcessDeprecated(out_mcm, process_handle, base_address, size);
        }
    }

    bool CanAddGuardRegionsInProcess(Handle process_handle, uintptr_t address, size_t size) {
        MemoryInfo mem_info;
        u32 page_info;

        /* Nintendo doesn't validate SVC return values at all. */
        /* TODO: Should we allow these to fail? */
        R_ABORT_UNLESS(svcQueryProcessMemory(&mem_info, &page_info, process_handle, address - 1));
        if (mem_info.type == MemType_Unmapped && address - GuardRegionSize >= mem_info.addr) {
            R_ABORT_UNLESS(svcQueryProcessMemory(&mem_info, &page_info, process_handle, address + size));
            return mem_info.type == MemType_Unmapped && address + size + GuardRegionSize <= mem_info.addr + mem_info.size;
        }

        return false;
    }

}
