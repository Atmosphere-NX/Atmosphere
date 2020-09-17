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

    /* Definitions. */
    constexpr size_t InvalidContextId = static_cast<size_t>(-1);

    /* Access utilities. */
    void SetDevelopmentHardware(bool is_development_hardware);
    void SetDevelopmentFunctionEnabled(bool is_development_function_enabled);
    bool IsDevelopmentHardware();
    bool IsDevelopmentFunctionEnabled();
    bool ShouldEaseNroRestriction();

    /* Context utilities. */
    Result RegisterProcess(size_t *out_context_id, os::ManagedHandle process_handle, os::ProcessId process_id);
    Result ValidateProcess(size_t context_id, os::ProcessId process_id);
    void   UnregisterProcess(size_t context_id);

    /* Service implementations. */
    Result RegisterModuleInfo(size_t context_id, os::ManagedHandle process_h, u64 nrr_address, u64 nrr_size, NrrKind nrr_kind, bool enforce_nrr_kind);
    Result UnregisterModuleInfo(size_t context_id, u64 nrr_address);
    Result MapManualLoadModuleMemory(u64 *out_address, size_t context_id, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size);
    Result UnmapManualLoadModuleMemory(size_t context_id, u64 nro_address);

    /* Debug service implementations. */
    Result GetProcessModuleInfo(u32 *out_count, LoaderModuleInfo *out_infos, size_t max_out_count, os::ProcessId process_id);

}
