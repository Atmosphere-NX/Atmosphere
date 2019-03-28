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

#pragma once
#include <switch.h>

struct FsPath {
    char str[FS_MAX_PATH];
};

constexpr FsPath EncodeConstantFsPath(const char *p) {
    FsPath path = {0};
    for (size_t i = 0; i < sizeof(path) - 1; i++) {
        path.str[i] = p[i];
        if (p[i] == 0) {
            break;
        }
    }
    return path;
}

constexpr size_t GetConstantFsPathLen(FsPath path) {
    size_t len = 0;
    for (size_t i = 0; i < sizeof(path) - 1; i++) {
        if (path.str[i] == 0) {
            break;
        }
        len++;
    }
    return len;
}

class FsPathUtils {
    public:
        static constexpr FsPath RootPath = EncodeConstantFsPath("/");
    public:
        static Result VerifyPath(const char *path, size_t max_path_len, size_t max_name_len);
        static Result ConvertPathForServiceObject(FsPath *out, const char *path);
        
        static Result IsNormalized(bool *out, const char *path);
        static Result Normalize(char *out, size_t max_out_size, const char *src, size_t *out_size);

        static bool IsWindowsDriveLetter(const char c) {
            return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
        }
        
        static bool IsCurrentDirectory(const char *path) {
            return path[0] == '.' && (path[1] == 0 || path[1] == '/');
        }
        
        static bool IsParentDirectory(const char *path) {
            return path[0] == '.' && path[1] == '.' && (path[2] == 0 || path[2] == '/');
        }
        
        static bool IsWindowsAbsolutePath(const char *path) {
            /* Nintendo uses this in path comparisons... */
            return IsWindowsDriveLetter(path[0]) && path[1] == ':';
        }
};
