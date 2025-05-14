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
#include "ldr_argument_store.hpp"

namespace ams::ldr {

    /* Process Creation API. */
    Result CreateProcess(os::NativeHandle *out, PinId pin_id, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &override_status, const char *path, const ArgumentStore::Entry *argument, u32 flags, os::NativeHandle resource_limit, const ldr::ProgramAttributes &attrs);
    Result GetProgramInfo(ProgramInfo *out, cfg::OverrideStatus *out_status, const ncm::ProgramLocation &loc, const char *path, const ldr::ProgramAttributes &attrs);

    Result PinProgram(PinId *out_id, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &override_status);
    Result UnpinProgram(PinId id);

    Result GetProcessModuleInfo(u32 *out_count, ldr::ModuleInfo *out, size_t max_out_count, os::ProcessId process_id);

    Result GetProgramLocationAndOverrideStatusFromPinId(ncm::ProgramLocation *out, cfg::OverrideStatus *out_status, PinId pin_id);

}
