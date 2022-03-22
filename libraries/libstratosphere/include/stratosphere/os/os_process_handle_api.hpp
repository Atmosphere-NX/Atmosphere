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
#include <stratosphere/os/os_native_handle.hpp>
#include <stratosphere/ncm/ncm_program_id.hpp>

namespace ams::os {

    Result GetProcessId(os::ProcessId *out, NativeHandle handle);

    ALWAYS_INLINE ProcessId GetProcessId(NativeHandle handle) {
        ProcessId process_id;
        R_ABORT_UNLESS(GetProcessId(std::addressof(process_id), handle));
        return process_id;
    }

    ALWAYS_INLINE ProcessId GetCurrentProcessId() {
        return GetProcessId(GetCurrentProcessHandle());
    }

    Result GetProgramId(ncm::ProgramId *out, NativeHandle handle);

    ALWAYS_INLINE ncm::ProgramId GetProgramId(NativeHandle handle) {
        ncm::ProgramId program_id;
        R_ABORT_UNLESS(GetProgramId(std::addressof(program_id), handle));
        return program_id;
    }

    ALWAYS_INLINE ncm::ProgramId GetCurrentProgramId() {
        return GetProgramId(GetCurrentProcessHandle());
    }

}
