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
#include "ldr_launch_record.hpp"

namespace ams::ldr {

    namespace {

        /* Global cache. */
        std::set<u64> g_launched_programs;

    }

    /* Launch Record API. */
    bool HasLaunchedProgram(ncm::ProgramId program_id) {
        return g_launched_programs.find(program_id.value) != g_launched_programs.end();
    }

    void SetLaunchedProgram(ncm::ProgramId program_id) {
        g_launched_programs.insert(program_id.value);
    }

}

/* Loader wants to override this libstratosphere function, which is weakly linked. */
/* This is necessary to prevent circular dependencies. */
namespace ams::pm::info {

    Result HasLaunchedProgram(bool *out, ncm::ProgramId program_id) {
        *out = ldr::HasLaunchedProgram(program_id);
        return ResultSuccess();
    }

}
