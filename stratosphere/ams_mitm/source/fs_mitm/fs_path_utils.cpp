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

Result FsPathUtils::IsNormalized(bool *out, const char *path) {
    /* Nintendo uses a state machine here. */
    enum class PathState {
        Start,
        Normal,
        FirstSeparator,
        Separator,
        CurrentDir,
        ParentDir,
        WindowsDriveLetter,
    };
    
    PathState state = PathState::Start;
    
    for (const char *cur = path; *cur != 0; cur++) {
        const char c = *cur;
        switch (state) {
            case PathState::Start:
                if (IsWindowsDriveLetter(c)) {
                    state = PathState::WindowsDriveLetter;
                } else if (c == '/') {
                    state = PathState::FirstSeparator;
                } else {
                    return ResultFsInvalidPathFormat;
                }
                break;
            case PathState::Normal:
                if (c == '/') {
                    state = PathState::Separator;
                }
                break;
            case PathState::FirstSeparator:
            case PathState::Separator:
                /* It is unclear why first separator and separator are separate states... */
                if (c == '/') {
                    *out = false;
                    return 0;
                } else if (c == '.') {
                    state = PathState::CurrentDir;
                } else {
                    state = PathState::Normal;
                }
                break;
            case PathState::CurrentDir:
                if (c == '/') {
                    *out = false;
                    return 0;
                } else if (c == '.') {
                    state = PathState::ParentDir;
                } else {
                    state = PathState::Normal;
                }
                break;
            case PathState::ParentDir:
                if (c == '/') {
                    *out = false;
                    return 0;
                } else {
                    state = PathState::Normal;
                }
                break;
            case PathState::WindowsDriveLetter:
                if (c == ':') {
                    *out = true;
                    return 0;
                } else {
                    return ResultFsInvalidPathFormat;
                }
                break;
        }
    }
    
    switch (state) {
        case PathState::Start:
        case PathState::WindowsDriveLetter:
            return ResultFsInvalidPathFormat;
        case PathState::FirstSeparator:
        case PathState::Separator:
            *out = false;
            break;
        case PathState::Normal:
        case PathState::CurrentDir:
        case PathState::ParentDir:
            *out = true;
            break;
    }
    
    return 0;
}

Result FsPathUtils::Normalize(char *out, size_t max_out_size, const char *src, size_t *out_size) {
    /* TODO */
    return ResultFsNotImplemented;
}
