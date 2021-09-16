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
#include "sprofile_srv_profile_manager.hpp"

namespace ams::sprofile::srv {

    Result WriteFile(const char *path, const void *src, size_t size) {
        /* Create the file. */
        R_TRY_CATCH(fs::CreateFile(path, size)) {
            R_CATCH(fs::ResultPathAlreadyExists) { /* It's okay if the file already exists. */ }
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        /* Open the file. */
        fs::FileHandle file;
        R_ABORT_UNLESS(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Write));
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        /* Set the file size. */
        R_ABORT_UNLESS(fs::SetFileSize(file, size));

        /* Write the file. */
        return fs::WriteFile(file, 0, src, size, fs::WriteOption::Flush);
    }

}
