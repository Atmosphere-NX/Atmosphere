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
#include "ini.h"

namespace ams::util::ini {

    /* Ensure that types are the same for Handler vs ini_handler. */
    static_assert(std::is_same<Handler, ::ini_handler>::value, "Bad ini::Handler definition!");

    namespace {

        struct FileContext {
            fs::FileHandle file;
            s64 offset;
            s64 num_left;

            explicit FileContext(fs::FileHandle f) : file(f), offset(0) {
                R_ABORT_UNLESS(fs::GetFileSize(std::addressof(this->num_left), this->file));
            }
        };

        struct IFileContext {
            fs::fsa::IFile *file;
            s64 offset;
            s64 num_left;

            explicit IFileContext(fs::fsa::IFile *f) : file(f), offset(0) {
                R_ABORT_UNLESS(file->GetSize(std::addressof(this->num_left)));
            }
        };

        char *ini_reader_file_handle(char *str, int num, void *stream) {
            FileContext *ctx = static_cast<FileContext *>(stream);

            if (ctx->num_left == 0 || num < 2) {
                return nullptr;
            }

            /* Read as many bytes as we can. */
            s64 cur_read = std::min<s64>(num - 1, ctx->num_left);
            R_ABORT_UNLESS(fs::ReadFile(ctx->file, ctx->offset, str, cur_read, fs::ReadOption()));

            /* Only "read" up to the first \n. */
            size_t offset = cur_read;
            for (auto i = 0; i < cur_read; i++) {
                if (str[i] == '\n') {
                    offset = i + 1;
                    break;
                }
            }

            /* Ensure null termination. */
            str[offset] = '\0';

            /* Update context. */
            ctx->offset   += offset;
            ctx->num_left -= offset;

            return str;
        }

        char *ini_reader_ifile(char *str, int num, void *stream) {
            IFileContext *ctx = static_cast<IFileContext *>(stream);

            if (ctx->num_left == 0 || num < 2) {
                return nullptr;
            }

            /* Read as many bytes as we can. */
            s64 cur_read = std::min<s64>(num - 1, ctx->num_left);
            size_t read;
            R_ABORT_UNLESS(ctx->file->Read(std::addressof(read), ctx->offset, str, cur_read, fs::ReadOption()));
            AMS_ABORT_UNLESS(static_cast<s64>(read) == cur_read);

            /* Only "read" up to the first \n. */
            size_t offset = cur_read;
            for (auto i = 0; i < cur_read; i++) {
                if (str[i] == '\n') {
                    offset = i + 1;
                    break;
                }
            }

            /* Ensure null termination. */
            str[offset] = '\0';

            /* Update context. */
            ctx->offset   += offset;
            ctx->num_left -= offset;

            return str;
        }

    }

    /* Utilities for dealing with INI file configuration. */
    int ParseString(const char *ini_str, void *user_ctx, Handler h) {
        return ini_parse_string(ini_str, h, user_ctx);
    }

    int ParseFile(fs::FileHandle file, void *user_ctx, Handler h) {
        FileContext ctx(file);
        return ini_parse_stream(ini_reader_file_handle, &ctx, h, user_ctx);
    }

    int ParseFile(fs::fsa::IFile *file, void *user_ctx, Handler h) {
        IFileContext ctx(file);
        return ini_parse_stream(ini_reader_ifile, &ctx, h, user_ctx);
    }

}