/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include <cstring>
#include <switch.h>
#include "fs_path_utils.hpp"
#include "fs_results.hpp"

Result FsPathUtils::VerifyPath(const char *path, size_t max_path_len, size_t max_name_len) {
    const char *cur = path;
    size_t name_len = 0;

    for (size_t path_len = 0; path_len <= max_path_len && name_len <= max_name_len; path_len++) {
        const char c = *cur;
        /* If terminated, we're done. */
        if (c == 0) {
            return 0;
        }

        /* TODO: Nintendo converts the path from utf-8 to utf-32, one character at a time. */
        /* We should do this. */

        /* Banned characters: :*?<>| */
        if (c == ':' || c == '*' || c == '?' || c == '<' || c == '>' || c == '|') {
            return ResultFsInvalidCharacter;
        }

        name_len++;
        /* Check for separator. */
        if (c == '/' || c == '\\') {
            name_len = 0;
        }
    }

    return ResultFsTooLongPath;
}

Result FsPathUtils::ConvertPathForServiceObject(FsPath *out, const char *path) {
    /* Check for nullptr. */
    if (out == nullptr || path == nullptr) {
        return ResultFsInvalidPath;
    }

    /* Copy string, NULL terminate. */
    /* NOTE: Nintendo adds an extra char at 0x301 for NULL terminator */
    /* But then forces 0x300 to NULL anyway... */
    std::strncpy(out->str, path, sizeof(out->str) - 1);
    out->str[sizeof(out->str)-1] = 0;

    /* Replace any instances of \ with / */
    for (size_t i = 0; i < sizeof(out->str); i++) {
        if (out->str[i] == '\\') {
            out->str[i] = '/';
        }
    }

    /* Nintendo allows some liberties if the path is a windows path. Who knows why... */
    const auto prefix_len = FsPathUtils::IsWindowsAbsolutePath(path) ? 2 : 0;
    const size_t max_len = (FS_MAX_PATH-1) - prefix_len;
    return FsPathUtils::VerifyPath(out->str + prefix_len, max_len, max_len);
}
