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

        constexpr bool IsValidMapCodeMemoryPermission(ams::svc::MemoryPermission perm) {
            return perm == ams::svc::MemoryPermission_ReadWrite;
        }

        constexpr bool IsValidMapToOwnerCodeMemoryPermission(ams::svc::MemoryPermission perm) {
            return perm == ams::svc::MemoryPermission_Read || perm == ams::svc::MemoryPermission_ReadExecute;
        }

        constexpr bool IsValidUnmapCodeMemoryPermission(ams::svc::MemoryPermission perm) {
            return perm == ams::svc::MemoryPermission_None;
        }

        constexpr bool IsValidUnmapFromOwnerCodeMemoryPermission(ams::svc::MemoryPermission perm) {
            return perm == ams::svc::MemoryPermission_None;
        }

        Result CreateCodeMemory(ams::svc::Handle *out, uintptr_t address, size_t size) {
            /* Validate address / size. */
            R_UNLESS(util::IsAligned(address, PageSize), svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size,    PageSize), svc::ResultInvalidSize());
            R_UNLESS(size > 0,                           svc::ResultInvalidSize());
            R_UNLESS((address < address + size),         svc::ResultInvalidCurrentMemory());

            /* Create the code memory. */
            KCodeMemory *code_mem = KCodeMemory::Create();
            R_UNLESS(code_mem != nullptr, svc::ResultOutOfResource());
            ON_SCOPE_EXIT { code_mem->Close(); };

            /* Verify that the region is in range. */
            R_UNLESS(GetCurrentProcess().GetPageTable().Contains(address, size), svc::ResultInvalidCurrentMemory());

            /* Initialize the code memory. */
            R_TRY(code_mem->Initialize(address, size));

            /* Register the code memory. */
            KCodeMemory::Register(code_mem);

            /* Add the code memory to the handle table. */
            R_TRY(GetCurrentProcess().GetHandleTable().Add(out, code_mem));

            return ResultSuccess();
        }

        Result ControlCodeMemory(ams::svc::Handle code_memory_handle, ams::svc::CodeMemoryOperation operation, uint64_t address, uint64_t size, ams::svc::MemoryPermission perm) {
            /* Validate the address / size. */
            R_UNLESS(util::IsAligned(address, PageSize),         svc::ResultInvalidAddress());
            R_UNLESS(util::IsAligned(size,    PageSize),         svc::ResultInvalidSize());
            R_UNLESS(size > 0,                                   svc::ResultInvalidSize());
            R_UNLESS((address < address + size),                 svc::ResultInvalidCurrentMemory());
            R_UNLESS(address == static_cast<uintptr_t>(address), svc::ResultInvalidCurrentMemory());
            R_UNLESS(size == static_cast<size_t>(size),          svc::ResultInvalidCurrentMemory());

            /* Get the code memory from its handle. */
            KScopedAutoObject code_mem = GetCurrentProcess().GetHandleTable().GetObject<KCodeMemory>(code_memory_handle);
            R_UNLESS(code_mem.IsNotNull(), svc::ResultInvalidHandle());

            /* NOTE: Here, Atmosphere extends the SVC to allow code memory operations on one's own process. */
            /* This enables homebrew usage of these SVCs for JIT. */
            /* R_UNLESS(code_mem->GetOwner() != GetCurrentProcessPointer(), svc::ResultInvalidHandle()); */

            /* Perform the operation. */
            switch (operation) {
                case ams::svc::CodeMemoryOperation_Map:
                    {
                        /* Check that the region is in range. */
                        R_UNLESS(GetCurrentProcess().GetPageTable().CanContain(address, size, KMemoryState_CodeOut), svc::ResultInvalidMemoryRegion());

                        /* Check the memory permission. */
                        R_UNLESS(IsValidMapCodeMemoryPermission(perm), svc::ResultInvalidNewMemoryPermission());

                        /* Map the memory. */
                        R_TRY(code_mem->Map(address, size));
                    }
                    break;
                case ams::svc::CodeMemoryOperation_Unmap:
                    {
                        /* Check that the region is in range. */
                        R_UNLESS(GetCurrentProcess().GetPageTable().CanContain(address, size, KMemoryState_CodeOut), svc::ResultInvalidMemoryRegion());

                        /* Check the memory permission. */
                        R_UNLESS(IsValidUnmapCodeMemoryPermission(perm), svc::ResultInvalidNewMemoryPermission());

                        /* Unmap the memory. */
                        R_TRY(code_mem->Unmap(address, size));
                    }
                    break;
                case ams::svc::CodeMemoryOperation_MapToOwner:
                    {
                        /* Check that the region is in range. */
                        R_UNLESS(code_mem->GetOwner()->GetPageTable().CanContain(address, size, KMemoryState_GeneratedCode), svc::ResultInvalidMemoryRegion());

                        /* Check the memory permission. */
                        R_UNLESS(IsValidMapToOwnerCodeMemoryPermission(perm), svc::ResultInvalidNewMemoryPermission());

                        /* Map the memory to its owner. */
                        R_TRY(code_mem->MapToOwner(address, size, perm));
                    }
                    break;
                case ams::svc::CodeMemoryOperation_UnmapFromOwner:
                    {
                        /* Check that the region is in range. */
                        R_UNLESS(code_mem->GetOwner()->GetPageTable().CanContain(address, size, KMemoryState_GeneratedCode), svc::ResultInvalidMemoryRegion());

                        /* Check the memory permission. */
                        R_UNLESS(IsValidUnmapFromOwnerCodeMemoryPermission(perm), svc::ResultInvalidNewMemoryPermission());

                        /* Unmap the memory from its owner. */
                        R_TRY(code_mem->UnmapFromOwner(address, size));
                    }
                    break;
                default:
                    return svc::ResultInvalidEnumValue();
            }

            return ResultSuccess();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result CreateCodeMemory64(ams::svc::Handle *out_handle, ams::svc::Address address, ams::svc::Size size) {
        return CreateCodeMemory(out_handle, address, size);
    }

    Result ControlCodeMemory64(ams::svc::Handle code_memory_handle, ams::svc::CodeMemoryOperation operation, uint64_t address, uint64_t size, ams::svc::MemoryPermission perm) {
        return ControlCodeMemory(code_memory_handle, operation, address, size, perm);
    }

    /* ============================= 64From32 ABI ============================= */

    Result CreateCodeMemory64From32(ams::svc::Handle *out_handle, ams::svc::Address address, ams::svc::Size size) {
        return CreateCodeMemory(out_handle, address, size);
    }

    Result ControlCodeMemory64From32(ams::svc::Handle code_memory_handle, ams::svc::CodeMemoryOperation operation, uint64_t address, uint64_t size, ams::svc::MemoryPermission perm) {
        return ControlCodeMemory(code_memory_handle, operation, address, size, perm);
    }

}
