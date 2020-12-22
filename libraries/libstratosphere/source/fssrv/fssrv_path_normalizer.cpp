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
        /* Check pre-conditions. */
        AMS_ASSERT(out_path != nullptr);
        AMS_ASSERT(out_buf  != nullptr);

        /* Clear output. */
        *out_path = nullptr;
        *out_buf = Buffer();

        /* Check if we're normalized. */
        bool normalized = false;
        R_TRY(fs::PathNormalizer::IsNormalized(std::addressof(normalized), path, preserve_unc, has_mount_name));

        if (normalized) {
            /* If we're already normalized, no allocation is needed. */
            *out_path = path;
        } else {
            /* Allocate a new buffer. */
            auto buffer = fs::impl::MakeUnique<char[]>(fs::EntryNameLengthMax + 1);
            R_UNLESS(buffer != nullptr, fs::ResultAllocationFailureInPathNormalizer());

            /* Generate normalized path. */
            size_t normalized_len = 0;
            R_TRY(fs::PathNormalizer::Normalize(buffer.get(), std::addressof(normalized_len), path, fs::EntryNameLengthMax + 1, preserve_unc, has_mount_name));

            /* Preserve the tail separator, if we should. */
            if (preserve_tail_sep) {
                if (fs::PathNormalizer::IsSeparator(path[strnlen(path, fs::EntryNameLengthMax) - 1]) && !fs::PathNormalizer::IsSeparator(buffer[normalized_len - 1])) {
                    AMS_ASSERT(normalized_len < fs::EntryNameLengthMax);
                    buffer[normalized_len]     = fs::StringTraits::DirectorySeparator;
                    buffer[normalized_len + 1] = fs::StringTraits::NullTerminator;
                }
            }

            /* Save output. */
            *out_path = buffer.get();
            *out_buf = std::move(buffer);
        }

        return ResultSuccess();
    }

}
