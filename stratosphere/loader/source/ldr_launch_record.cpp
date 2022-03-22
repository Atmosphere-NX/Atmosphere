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
#include "ldr_launch_record.hpp"

namespace ams::ldr {

    namespace {

        static constexpr size_t MaxBootPrograms = 0x50;
        constinit std::array<ncm::ProgramId, MaxBootPrograms> g_launched_boot_programs = [] {
            std::array<ncm::ProgramId, MaxBootPrograms> arr = {};
            for (size_t i = 0; i < MaxBootPrograms; ++i) {
                arr[i] = ncm::InvalidProgramId;
            }
            return arr;
        }();

        constinit bool g_boot_programs_done = false;

        bool HasLaunchedBootProgramImpl(ncm::ProgramId program_id) {
            for (const auto &launched : g_launched_boot_programs) {
                if (launched == program_id) {
                    return true;
                }
            }
            return false;
        }

        void SetLaunchedBootProgramImpl(ncm::ProgramId program_id) {
            for (size_t i = 0; i < MaxBootPrograms; ++i) {
                if (g_launched_boot_programs[i] == ncm::InvalidProgramId) {
                    g_launched_boot_programs[i] = program_id;
                    return;
                }
            }
            AMS_ABORT("Too many boot programs");
        }

    }

    /* Launch Record API. */
    bool HasLaunchedBootProgram(ncm::ProgramId program_id) {
        return HasLaunchedBootProgramImpl(program_id);
    }

    void SetLaunchedBootProgram(ncm::ProgramId program_id) {
        if (!g_boot_programs_done) {
            SetLaunchedBootProgramImpl(program_id);
            if (program_id == ncm::SystemAppletId::Qlaunch) {
                g_boot_programs_done = true;
            }
        }
    }

}

/* Loader wants to override this libstratosphere function, which is weakly linked. */
/* This is necessary to prevent circular dependencies. */
namespace ams::pm::info {

    Result HasLaunchedBootProgram(bool *out, ncm::ProgramId program_id) {
        *out = ldr::HasLaunchedBootProgram(program_id);
        return ResultSuccess();
    }

}
