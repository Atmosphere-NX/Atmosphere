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
#include "ldr_launch_record.hpp"

namespace ams::ldr {

    namespace {

        /* Global cache. */
        std::set<u64> g_launched_programs;

        constexpr size_t NumSystemProgramIds = ncm::SystemProgramId::End.value - ncm::SystemProgramId::Start.value + 1;
        static_assert(util::IsPowerOfTwo(NumSystemProgramIds));
        static_assert(util::IsAligned(NumSystemProgramIds, BITSIZEOF(u64)));

        bool IsTrackableSystemProgramId(ncm::ProgramId program_id) {
            return ncm::SystemProgramId::Start <= program_id && program_id <= ncm::SystemProgramId::End;
        }

        u64 g_system_launch_records[NumSystemProgramIds / BITSIZEOF(u64)];

        void SetLaunchedSystemProgram(ncm::SystemProgramId program_id) {
            const u64 val = program_id.value - ncm::SystemProgramId::Start.value;
            g_system_launch_records[val / BITSIZEOF(u64)] |= (1ul << (val % BITSIZEOF(u64)));
        }

        bool HasLaunchedSystemProgram(ncm::SystemProgramId program_id) {
            const u64 val = program_id.value - ncm::SystemProgramId::Start.value;
            return (g_system_launch_records[val / BITSIZEOF(u64)] & (1ul << (val % BITSIZEOF(u64)))) != 0;
        }

    }

    /* Launch Record API. */
    bool HasLaunchedProgram(ncm::ProgramId program_id) {
        if (IsTrackableSystemProgramId(program_id)) {
            return HasLaunchedSystemProgram(ncm::SystemProgramId{program_id.value});
        } else {
            return g_launched_programs.find(program_id.value) != g_launched_programs.end();
        }
    }

    void SetLaunchedProgram(ncm::ProgramId program_id) {
        if (IsTrackableSystemProgramId(program_id)) {
            SetLaunchedSystemProgram(ncm::SystemProgramId{program_id.value});
        } else {
            g_launched_programs.insert(static_cast<u64>(program_id));
        }
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
