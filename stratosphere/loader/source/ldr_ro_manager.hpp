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

namespace ams::ldr::ro {

    /* RO Manager API. */
    Result PinProgram(PinId *out, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &status);
    Result UnpinProgram(PinId id);
    Result GetProgramLocationAndStatus(ncm::ProgramLocation *out, cfg::OverrideStatus *out_status, PinId id);
    Result RegisterProcess(PinId id, os::ProcessId process_id, ncm::ProgramId program_id);
    Result RegisterModule(PinId id, const u8 *build_id, uintptr_t address, size_t size);
    Result GetProcessModuleInfo(u32 *out_count, ModuleInfo *out, size_t max_out_count, os::ProcessId process_id);

}
