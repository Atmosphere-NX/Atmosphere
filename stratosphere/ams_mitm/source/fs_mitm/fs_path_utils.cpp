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
#include <cstdlib>
#include <switch.h>
#include <stratosphere.hpp>

#include "fs_path_utils.hpp"

Result FsPathUtils::VerifyPath(const char *path, size_t max_path_len, size_t max_name_len) {
    const char *cur = path;
    size_t name_len = 0;

    for (size_t path_len = 0; path_len <= max_path_len && name_len <= max_name_len; path_len++) {
        const char c = *(cur++);
        /* If terminated, we're done. */
        if (c == 0) {
            return ResultSuccess;
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
                    return ResultSuccess;
                } else if (c == '.') {
                    state = PathState::CurrentDir;
                } else {
                    state = PathState::Normal;
                }
                break;
            case PathState::CurrentDir:
                if (c == '/') {
                    *out = false;
                    return ResultSuccess;
                } else if (c == '.') {
                    state = PathState::ParentDir;
                } else {
                    state = PathState::Normal;
                }
                break;
            case PathState::ParentDir:
                if (c == '/') {
                    *out = false;
                    return ResultSuccess;
                } else {
                    state = PathState::Normal;
                }
                break;
            case PathState::WindowsDriveLetter:
                if (c == ':') {
                    *out = true;
                    return ResultSuccess;
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
        case PathState::Normal:
            *out = true;
            break;
        case PathState::CurrentDir:
        case PathState::ParentDir:
        case PathState::Separator:
            *out = false;
            break;
    }
    
    return ResultSuccess;
}

Result FsPathUtils::Normalize(char *out, size_t max_out_size, const char *src, size_t *out_len) {
    /* Paths must start with / */
    if (src[0] != '/') {
        return ResultFsInvalidPathFormat;
    }
    
    bool skip_next_sep = false;
    size_t i = 0;
    size_t len = 0;
    
    while (src[i] != 0) {
        if (src[i] == '/') {
            /* Swallow separators. */
            while (src[++i] == '/') { }
            if (src[i] == 0) {
                break;
            }
            
            /* Handle skip if needed */
            if (!skip_next_sep) {
                if (len + 1 == max_out_size) {
                    out[len] = 0;
                    if (out_len != nullptr) {
                        *out_len = len;
                    }
                    return ResultFsTooLongPath;
                }
                
                out[len++] = '/';
                
                /* TODO: N has some weird windows support stuff here under a bool. */
                /* Boolean is normally false though? */
            }
            skip_next_sep = false;
        }
        
        /* See length of current dir. */
        size_t dir_len = 0;
        while (src[i+dir_len] != '/' && src[i+dir_len] != 0) {
            dir_len++;
        }
        
        if (FsPathUtils::IsCurrentDirectory(&src[i])) {
            skip_next_sep = true;
        } else if (FsPathUtils::IsParentDirectory(&src[i])) {
            if (len == 1) {
                return ResultFsDirectoryUnobtainable;
            }
            
            /* Walk up a directory. */
            len -= 2;
            while (out[len] != '/') {
                len--;
            }
        } else {
            /* Copy, possibly truncating. */
            if (len + dir_len + 1 <= max_out_size) {
                for (size_t j = 0; j < dir_len; j++) {
                    out[len++] = src[i+j];
                }
            } else {
                const size_t copy_len = max_out_size - 1 - len;
                for (size_t j = 0; j < copy_len; j++) {
                    out[len++] = src[i+j];
                }
                out[len] = 0;
                if (out_len != nullptr) {
                    *out_len = len;
                }
                return ResultFsTooLongPath;
            }
        }
        
        i += dir_len;
    }
    
    if (skip_next_sep) {
        len--;
    }
    
    if (len == 0 && max_out_size) {
        out[len++] = '/';
    }
    
    if (max_out_size < len - 1) {
        return ResultFsTooLongPath;
    }
    
    /* NULL terminate. */
    out[len] = 0;
    if (out_len != nullptr) {
        *out_len = len;
    }
    
    /* Assert normalized. */
    bool normalized = false;
    if (R_FAILED(FsPathUtils::IsNormalized(&normalized, out)) || !normalized) {
        std::abort();
    }
    
    return ResultSuccess;
}
