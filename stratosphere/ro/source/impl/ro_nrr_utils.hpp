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

#pragma once
#include <stratosphere.hpp>

namespace ams::ro::impl {

    /* Utilities for working with NRRs. */
    Result MapAndValidateNrr(NrrHeader **out_header, u64 *out_mapped_code_address, void *out_hash, size_t out_hash_size, Handle process_handle, ncm::ProgramId program_id, u64 nrr_heap_address, u64 nrr_heap_size, NrrKind nrr_kind, bool enforce_nrr_kind);
    Result UnmapNrr(Handle process_handle, const NrrHeader *header, u64 nrr_heap_address, u64 nrr_heap_size, u64 mapped_code_address);

    bool ValidateNrrHashTableEntry(const void *signed_area, size_t signed_area_size, size_t hashes_offset, size_t num_hashes, const void *nrr_hash, const u8 *hash_table, const void *desired_hash);

}