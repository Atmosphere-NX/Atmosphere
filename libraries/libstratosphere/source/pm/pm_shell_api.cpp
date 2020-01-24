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

namespace ams::pm::shell {

    /* Shell API. */
    Result WEAK_SYMBOL LaunchProgram(os::ProcessId *out_process_id, const ncm::ProgramLocation &loc, u32 launch_flags) {
        static_assert(sizeof(ncm::ProgramLocation) == sizeof(NcmProgramLocation));
        static_assert(alignof(ncm::ProgramLocation) == alignof(NcmProgramLocation));
        return pmshellLaunchProgram(launch_flags, reinterpret_cast<const NcmProgramLocation *>(&loc), reinterpret_cast<u64 *>(out_process_id));
    }

}
