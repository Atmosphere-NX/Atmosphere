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

#include <stdio.h>
#include <sys/stat.h>
#include "lib/fatfs/ff.h"
#include "fs_utils.h"
#include "fs_dev.h"

size_t get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == -1) {
        return 0;
    }

    return (size_t)st.st_size;
}

size_t read_from_file(void *dst, size_t dst_size, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        return 0;
    } else {
        size_t sz = fread(dst, 1, dst_size, file);
        fclose(file);
        return sz;
    }
}

size_t dump_to_file(const void *src, size_t src_size, const char *filename) {
    FILE *file = fopen(filename, "wb+");
    if (file == NULL) {
        return 0;
    } else {
        size_t sz = fwrite(src, 1, src_size, file);
        fclose(file);
        return sz;
    }
}

bool is_valid_folder(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }

    return false;
}

bool is_valid_file(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
        return true;
    }

    return false;
}

bool is_valid_concatenation_file(const char *path) {
    if (is_valid_file(path)) {
        return true;
    } else if (is_valid_folder(path)) {
        /* Check if the archive bit is set. */
        int rc = fsdev_get_attr(path);

        /* Failed to get file DOS attributes. */
        if (rc == -1) {
            return false;
        }

        /* Check if our path is not a directory (it should be if we're in this code, though...). */
        if (!(rc & AM_DIR)) {
            return false;
        }

        return (rc & AM_ARC) != 0;
    } else {
        return false;
    }
}
