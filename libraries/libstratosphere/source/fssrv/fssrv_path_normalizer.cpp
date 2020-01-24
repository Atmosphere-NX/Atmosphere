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

namespace ams::fssrv {

    Result PathNormalizer::Normalize(const char **out_path, Buffer *out_buf, const char *path, bool preserve_unc, bool preserve_tail_sep, bool has_mount_name) {
        /* Clear output. */
        *out_path = nullptr;
        *out_buf = Buffer();

        /* Find start of path. */
        const char *path_start = path;
        if (has_mount_name) {
            while (path_start < path + fs::MountNameLengthMax + 1) {
                if (fssystem::PathTool::IsNullTerminator(*path_start)) {
                    break;
                } else if (fssystem::PathTool::IsDriveSeparator(*(path_start++))) {
                    break;
                }
            }
            R_UNLESS(path < path_start - 1,                                   fs::ResultInvalidPath());
            R_UNLESS(fssystem::PathTool::IsDriveSeparator(*(path_start - 1)), fs::ResultInvalidPath());
        }

        /* Check if we're normalized. */
        bool normalized = false;
        R_TRY(fssystem::PathTool::IsNormalized(&normalized, path_start));

        if (normalized) {
            /* If we're already normalized, no allocation is needed. */
            *out_path = path;
        } else {
            /* Allocate a new buffer. */
            auto buffer = std::make_unique<char[]>(fs::EntryNameLengthMax + 1);
            R_UNLESS(buffer != nullptr, fs::ResultAllocationFailureInPathNormalizer());

            /* Copy in mount name. */
            const size_t mount_name_len = path_start - path;
            std::memcpy(buffer.get(), path, mount_name_len);

            /* Generate normalized path. */
            size_t normalized_len = 0;
            R_TRY(fssystem::PathTool::Normalize(buffer.get() + mount_name_len, &normalized_len, path_start, fs::EntryNameLengthMax + 1 - mount_name_len, preserve_unc));

            /* Preserve the tail separator, if we should. */
            if (preserve_tail_sep) {
                if (fssystem::PathTool::IsSeparator(path[strnlen(path, fs::EntryNameLengthMax) - 1])) {
                    /* Nintendo doesn't actually validate this. */
                    R_UNLESS(mount_name_len + normalized_len < fs::EntryNameLengthMax, fs::ResultTooLongPath());
                    buffer[mount_name_len + normalized_len]     = fssystem::StringTraits::DirectorySeparator;
                    buffer[mount_name_len + normalized_len + 1] = fssystem::StringTraits::NullTerminator;
                }
            }

            /* Save output. */
            *out_path = buffer.get();
            *out_buf = std::move(buffer);
        }

        return ResultSuccess();
    }

}
