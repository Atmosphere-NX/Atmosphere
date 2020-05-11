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
#include "ldr_arguments.hpp"

namespace ams::ldr::args {

    namespace {

        /* Convenience definitions. */
        constexpr size_t MaxArgumentInfos = 10;

        /* Global storage. */
        ArgumentInfo g_argument_infos[MaxArgumentInfos];

        /* Helpers. */
        ArgumentInfo *FindArgumentInfo(ncm::ProgramId program_id) {
            for (size_t i = 0; i < MaxArgumentInfos; i++) {
                if (g_argument_infos[i].program_id == program_id) {
                    return &g_argument_infos[i];
                }
            }
            return nullptr;
        }

        ArgumentInfo *FindFreeArgumentInfo() {
            return FindArgumentInfo(ncm::InvalidProgramId);
        }

    }

    /* API. */
    const ArgumentInfo *Get(ncm::ProgramId program_id) {
        return FindArgumentInfo(program_id);
    }

    Result Set(ncm::ProgramId program_id, const void *args, size_t args_size) {
        R_UNLESS(args_size < ArgumentSizeMax, ldr::ResultTooLongArgument());

        ArgumentInfo *arg_info = FindArgumentInfo(program_id);
        if (arg_info == nullptr) {
            arg_info = FindFreeArgumentInfo();
        }
        R_UNLESS(arg_info != nullptr, ldr::ResultTooManyArguments());

        arg_info->program_id = program_id;
        arg_info->args_size = args_size;
        std::memcpy(arg_info->args, args, args_size);
        return ResultSuccess();
    }

    Result Flush() {
        for (size_t i = 0; i < MaxArgumentInfos; i++) {
            g_argument_infos[i].program_id = ncm::InvalidProgramId;
        }
        return ResultSuccess();
    }

}
