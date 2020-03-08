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

namespace ams::fs {

    Result VerifyPath(const char *path, size_t max_path_len, size_t max_name_len) {
        const char *cur = path;
        size_t name_len = 0;

        for (size_t path_len = 0; path_len <= max_path_len && name_len <= max_name_len; path_len++) {
            const char c = *(cur++);

            /* If terminated, we're done. */
            R_SUCCEED_IF(PathTool::IsNullTerminator(c));

            /* TODO: Nintendo converts the path from utf-8 to utf-32, one character at a time. */
            /* We should do this. */

            /* Banned characters: :*?<>| */
            R_UNLESS((c != ':' && c != '*' && c != '?' && c != '<' && c != '>' && c != '|'), fs::ResultInvalidCharacter());

            name_len++;
            /* Check for separator. */
            if (c == '\\' || c == '/') {
                name_len = 0;
            }

        }

        return fs::ResultTooLongPath();
    }

}
