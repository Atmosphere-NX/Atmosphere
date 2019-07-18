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
#include <stratosphere/util.hpp>

#include "ini.h"

namespace sts::util::ini {

    /* Ensure that types are the same for Handler vs ini_handler. */
    static_assert(std::is_same<Handler, ::ini_handler>::value, "Bad ini::Handler definition!");

    namespace {

        struct FsFileContext {
            FsFile *f;
            size_t offset;
            size_t num_left;

            explicit FsFileContext(FsFile *f) : f(f), offset(0) {
                u64 size;
                R_ASSERT(fsFileGetSize(this->f, &size));
                this->num_left = size_t(size);
            }
        };

        char *ini_reader_fs_file(char *str, int num, void *stream) {
            FsFileContext *ctx = reinterpret_cast<FsFileContext *>(stream);

            if (ctx->num_left == 0 || num < 2) {
                return nullptr;
            }

            /* Read as many bytes as we can. */
            size_t try_read = std::min(size_t(num - 1), ctx->num_left);
            size_t actually_read;
            R_ASSERT(fsFileRead(ctx->f, ctx->offset, str, try_read, FS_READOPTION_NONE, &actually_read));
            if (actually_read != try_read) {
                std::abort();
            }

            /* Only "read" up to the first \n. */
            size_t offset = actually_read;
            for (size_t i = 0; i < actually_read; i++) {
                if (str[i] == '\n') {
                    offset = i + 1;
                    break;
                }
            }

            /* Ensure null termination. */
            str[offset] = '\0';

            /* Update context. */
            ctx->offset += offset;
            ctx->num_left -= offset;

            return str;
        }

    }

    /* Utilities for dealing with INI file configuration. */
    int ParseString(const char *ini_str, void *user_ctx, Handler h) {
        return ini_parse_string(ini_str, h, user_ctx);
    }

    int ParseFile(FILE *f, void *user_ctx, Handler h) {
        return ini_parse_file(f, h, user_ctx);
    }

    int ParseFile(FsFile *f, void *user_ctx, Handler h) {
        FsFileContext ctx(f);
        return ini_parse_stream(ini_reader_fs_file, &ctx, h, user_ctx);
    }

    int ParseFile(const char *path, void *user_ctx, Handler h) {
        return ini_parse(path, h, user_ctx);
    }

}