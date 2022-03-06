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
#include <stratosphere.hpp>

namespace ams::ldr {

    Result InitializeForShell() {
        return ::ldrShellInitialize();
    }

    Result FinalizeForShell() {
        ::ldrShellExit();
        return ResultSuccess();
    }

    Result SetProgramArgument(ncm::ProgramId program_id, const void *arg, size_t size) {
        return ::ldrShellSetProgramArguments(static_cast<u64>(program_id), arg, size);
    }

    Result FlushArguments() {
        return ::ldrShellFlushArguments();
    }

}
