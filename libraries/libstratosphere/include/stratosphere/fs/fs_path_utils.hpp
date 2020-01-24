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
#pragma once
#include "fs_common.hpp"
#include "../fssrv/fssrv_sf_path.hpp"

namespace ams::fs {

    inline void Replace(char *dst, size_t dst_size, char old_char, char new_char) {
        for (char *cur = dst; cur < dst + dst_size && *cur != '\x00'; cur++) {
            if (*cur == old_char) {
                *cur = new_char;
            }
        }
    }

    inline Result FspPathPrintf(fssrv::sf::FspPath *dst, const char *format, ...) {
        /* Format the path. */
        std::va_list va_list;
        va_start(va_list, format);
        const size_t len = std::vsnprintf(dst->str, sizeof(dst->str), format, va_list);
        va_end(va_list);

        /* Validate length. */
        R_UNLESS(len < sizeof(dst->str), fs::ResultTooLongPath());

        /* Fix slashes. */
        Replace(dst->str, sizeof(dst->str) - 1, '\\', '/');

        return ResultSuccess();
    }

    Result VerifyPath(const char *path, size_t max_path_len, size_t max_name_len);

}
