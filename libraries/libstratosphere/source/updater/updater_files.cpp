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
#include "updater_files.hpp"

namespace ams::updater {

    Result ReadFile(size_t *out_size, void *dst, size_t dst_size, const char *path) {
        /* Open the file. */
        fs::FileHandle file;
        R_TRY_CATCH(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Read)) {
            R_CONVERT(fs::ResultPathNotFound, ResultInvalidBootImagePackage())
        } R_END_TRY_CATCH;
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        std::memset(dst, 0, dst_size);
        return fs::ReadFile(out_size, file, 0, dst, dst_size, fs::ReadOption());
    }

    Result GetFileHash(size_t *out_size, void *dst_hash, const char *path, void *work_buffer, size_t work_buffer_size) {
        /* Open the file. */
        fs::FileHandle file;
        R_TRY_CATCH(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Read)) {
            R_CONVERT(fs::ResultPathNotFound, ResultInvalidBootImagePackage())
        } R_END_TRY_CATCH;
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        /* Read in chunks, hashing as we go. */
        crypto::Sha256Generator generator;
        generator.Initialize();

        size_t total_size = 0;
        while (true) {
            size_t size;
            R_TRY(fs::ReadFile(std::addressof(size), file, total_size, work_buffer, work_buffer_size, fs::ReadOption()));

            generator.Update(work_buffer, size);
            total_size += size;

            if (size != work_buffer_size) {
                break;
            }
        }

        generator.GetHash(dst_hash, crypto::Sha256Generator::HashSize);
        *out_size = total_size;
        return ResultSuccess();
    }

}
