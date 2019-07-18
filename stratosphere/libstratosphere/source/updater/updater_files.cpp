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

#include <switch.h>
#include <stratosphere.hpp>

#include "updater_files.hpp"

namespace sts::updater {

    Result ReadFile(size_t *out_size, void *dst, size_t dst_size, const char *path) {
        FILE *fp = fopen(path, "rb");
        if (fp == NULL) {
            return ResultUpdaterInvalidBootImagePackage;
        }
        ON_SCOPE_EXIT { fclose(fp); };

        std::memset(dst, 0, dst_size);
        size_t read_size = fread(dst, 1, dst_size, fp);
        if (ferror(fp)) {
            return fsdevGetLastResult();
        }
        *out_size = read_size;
        return ResultSuccess;
    }

    Result GetFileHash(size_t *out_size, void *dst_hash, const char *path, void *work_buffer, size_t work_buffer_size) {
        FILE *fp = fopen(path, "rb");
        if (fp == NULL) {
            return ResultUpdaterInvalidBootImagePackage;
        }
        ON_SCOPE_EXIT { fclose(fp); };

        Sha256Context sha_ctx;
        sha256ContextCreate(&sha_ctx);

        size_t total_size = 0;
        while (true) {
            size_t read_size = fread(work_buffer, 1, work_buffer_size, fp);
            if (ferror(fp)) {
                return fsdevGetLastResult();
            }
            if (read_size == 0) {
                break;
            }

            sha256ContextUpdate(&sha_ctx, work_buffer, read_size);
            total_size += read_size;
            if (read_size != work_buffer_size) {
                break;
            }
        }

        sha256ContextGetHash(&sha_ctx, dst_hash);
        *out_size = total_size;
        return ResultSuccess;
    }

}
