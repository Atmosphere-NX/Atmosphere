/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include "ldr_arguments.hpp"

namespace sts::ldr::args {

    namespace {

        /* Convenience definitions. */
        constexpr ncm::TitleId FreeTitleId = {};
        constexpr size_t MaxArgumentInfos = 10;

        /* Global storage. */
        ArgumentInfo g_argument_infos[MaxArgumentInfos];

        /* Helpers. */
        ArgumentInfo *FindArgumentInfo(ncm::TitleId title_id) {
            for (size_t i = 0; i < MaxArgumentInfos; i++) {
                if (g_argument_infos[i].title_id == title_id) {
                    return &g_argument_infos[i];
                }
            }
            return nullptr;
        }

        ArgumentInfo *FindFreeArgumentInfo() {
            return FindArgumentInfo(FreeTitleId);
        }

    }

    /* API. */
    const ArgumentInfo *Get(ncm::TitleId title_id) {
        return FindArgumentInfo(title_id);
    }

    Result Set(ncm::TitleId title_id, const void *args, size_t args_size) {
        if (args_size >= ArgumentSizeMax) {
            return ResultLoaderTooLongArgument;
        }

        ArgumentInfo *arg_info = FindArgumentInfo(title_id);
        if (arg_info == nullptr) {
            arg_info = FindFreeArgumentInfo();
        }
        if (arg_info == nullptr) {
            return ResultLoaderTooManyArguments;
        }

        arg_info->title_id = title_id;
        arg_info->args_size = args_size;
        std::memcpy(arg_info->args, args, args_size);
        return ResultSuccess;
    }

    Result Clear() {
        for (size_t i = 0; i < MaxArgumentInfos; i++) {
            g_argument_infos[i].title_id = FreeTitleId;
        }
        return ResultSuccess;
    }

}
