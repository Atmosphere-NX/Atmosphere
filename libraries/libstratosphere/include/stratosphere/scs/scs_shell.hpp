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
#include <vapours.hpp>
#include <stratosphere/os.hpp>
#include <stratosphere/ncm/ncm_ids.hpp>

namespace ams::scs {

    using ProcessEventHandler = void(*)(u64 id, s32 socket, os::ProcessId process_id);

    void InitializeShell();

    void RegisterCommonProcessEventHandler(ProcessEventHandler on_start, ProcessEventHandler on_exit, ProcessEventHandler on_jit_debug);

    Result RegisterSocket(s32 socket, u64 id);
    void UnregisterSocket(s32 socket);

    Result LaunchProgram(os::ProcessId *out, const ncm::ProgramLocation &loc, const void *args, size_t args_size, u32 process_flags);

    inline Result LaunchProgram(os::ProcessId *out, ncm::ProgramId program_id, const void *args, size_t args_size, u32 process_flags) {
        return LaunchProgram(out, ncm::ProgramLocation::Make(program_id, ncm::StorageId::BuiltInSystem), args, args_size, process_flags);
    }

    Result SubscribeProcessEvent(s32 socket, bool is_register, u64 id);

}
