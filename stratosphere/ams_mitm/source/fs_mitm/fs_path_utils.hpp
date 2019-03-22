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

class FsPathUtils {
    public:
        static Result VerifyPath(const char *path, size_t max_path_len, size_t max_name_len);
        static Result ConvertPathForServiceObject(FsPath *out, const char *path);

        static bool IsWindowsAbsolutePath(const char *path) {
            /* Nintendo uses this in path comparisons... */
            return (('a' <= path[0] && path[0] <= 'z') || (('A' <= path[0] && path[0] <= 'Z'))) &&
                    path[0] != 0 && path[1] == ':';
        }
};
