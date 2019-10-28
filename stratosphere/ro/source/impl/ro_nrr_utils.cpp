/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "ro_nrr_utils.hpp"
#include "ro_service_impl.hpp"

namespace ams::ro::impl {

    namespace {

        /* Helper functions. */
        Result ValidateNrrSignature(const NrrHeader *header) {
            /* TODO: Implement RSA-2048 PSS..... */

            /* TODO: Check PSS fixed-key signature. */
            R_UNLESS(true, ResultNotAuthorized());

            /* Check ProgramId pattern is valid. */
            R_UNLESS(header->IsProgramIdValid(), ResultNotAuthorized());

            /* TODO: Check PSS signature over hashes. */
            R_UNLESS(true, ResultNotAuthorized());

            return ResultSuccess();
        }

        Result ValidateNrr(const NrrHeader *header, u64 size, ncm::ProgramId program_id, ModuleType expected_type, bool enforce_type) {
            /* Check magic. */
            R_UNLESS(header->IsMagicValid(), ResultInvalidNrr());

            /* Check size. */
            R_UNLESS(header->GetSize() == size, ResultInvalidSize());

            /* Only perform checks if we must. */
            const bool ease_nro_restriction = ShouldEaseNroRestriction();
            if (!ease_nro_restriction) {
                /* Check signature. */
                R_TRY(ValidateNrrSignature(header));

                /* Check program id. */
                R_UNLESS(header->GetProgramId() == program_id, ResultInvalidNrr());

                /* Check type. */
                if (hos::GetVersion() >= hos::Version_700 && enforce_type) {
                    R_UNLESS(header->GetType() == expected_type, ResultInvalidNrrType());
                }
            }

            return ResultSuccess();
        }

    }

    /* Utilities for working with NRRs. */
    Result MapAndValidateNrr(NrrHeader **out_header, u64 *out_mapped_code_address, Handle process_handle, ncm::ProgramId program_id, u64 nrr_heap_address, u64 nrr_heap_size, ModuleType expected_type, bool enforce_type) {
        map::MappedCodeMemory nrr_mcm(ResultInternalError{});

        /* First, map the NRR. */
        R_TRY(map::MapCodeMemoryInProcess(nrr_mcm, process_handle, nrr_heap_address, nrr_heap_size));

        const u64 code_address = nrr_mcm.GetDstAddress();
        uintptr_t map_address;
        R_UNLESS(R_SUCCEEDED(map::LocateMappableSpace(&map_address, nrr_heap_size)), ResultOutOfAddressSpace());

        /* Nintendo...does not check the return value of this map. We will check, instead of aborting if it fails. */
        map::AutoCloseMap nrr_map(map_address, process_handle, code_address, nrr_heap_size);
        R_TRY(nrr_map.GetResult());

        NrrHeader *nrr_header = reinterpret_cast<NrrHeader *>(map_address);
        R_TRY(ValidateNrr(nrr_header, nrr_heap_size, program_id, expected_type, enforce_type));

        /* Invalidation here actually prevents them from unmapping at scope exit. */
        nrr_map.Invalidate();
        nrr_mcm.Invalidate();

        *out_header = nrr_header;
        *out_mapped_code_address = code_address;
        return ResultSuccess();
    }

    Result UnmapNrr(Handle process_handle, const NrrHeader *header, u64 nrr_heap_address, u64 nrr_heap_size, u64 mapped_code_address) {
        R_TRY(svcUnmapProcessMemory(reinterpret_cast<void *>(const_cast<NrrHeader *>(header)), process_handle, mapped_code_address, nrr_heap_size));
        R_TRY(svcUnmapProcessCodeMemory(process_handle, mapped_code_address, nrr_heap_address, nrr_heap_size));
        return ResultSuccess();
    }

}
