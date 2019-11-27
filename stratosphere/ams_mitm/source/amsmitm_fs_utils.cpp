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
#include <stratosphere.hpp>
#include "amsmitm_initialization.hpp"
#include "amsmitm_fs_utils.hpp"

namespace ams::mitm::fs {

    namespace {

        /* Globals. */
        FsFileSystem g_sd_filesystem;

        /* Helpers. */
        Result EnsureSdInitialized() {
            R_UNLESS(serviceIsActive(&g_sd_filesystem.s), ams::fs::ResultSdCardNotPresent());
            return ResultSuccess();
        }

        void FormatAtmosphereSdPath(char *dst_path, size_t dst_path_size, const char *src_path) {
            if (src_path[0] == '/') {
                std::snprintf(dst_path, dst_path_size, "/atmosphere%s", src_path);
            } else {
                std::snprintf(dst_path, dst_path_size, "/atmosphere/%s", src_path);
            }
        }

        void FormatAtmosphereSdPath(char *dst_path, size_t dst_path_size, ncm::ProgramId program_id, const char *src_path) {
            if (src_path[0] == '/') {
                std::snprintf(dst_path, dst_path_size, "/atmosphere/contents/%016lx%s", static_cast<u64>(program_id), src_path);
            } else {
                std::snprintf(dst_path, dst_path_size, "/atmosphere/contents/%016lx/%s", static_cast<u64>(program_id), src_path);
            }
        }

    }

    void OpenGlobalSdCardFileSystem() {
        R_ASSERT(fsOpenSdCardFileSystem(&g_sd_filesystem));
    }

    Result OpenSdFile(FsFile *out, const char *path, u32 mode) {
        R_TRY(EnsureSdInitialized());
        return fsFsOpenFile(&g_sd_filesystem, path, mode, out);
    }

    Result OpenAtmosphereSdFile(FsFile *out, const char *path, u32 mode) {
        char fixed_path[FS_MAX_PATH];
        FormatAtmosphereSdPath(fixed_path, sizeof(fixed_path), path);
        return OpenSdFile(out, fixed_path, mode);
    }

    Result OpenAtmosphereSdFile(FsFile *out, ncm::ProgramId program_id, const char *path, u32 mode) {
        char fixed_path[FS_MAX_PATH];
        FormatAtmosphereSdPath(fixed_path, sizeof(fixed_path), program_id, path);
        return OpenSdFile(out, fixed_path, mode);
    }

}
