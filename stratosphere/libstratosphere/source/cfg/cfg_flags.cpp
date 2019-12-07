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

namespace ams::cfg {

    namespace {

        /* Helper. */
        bool HasFlagFile(const char *flag_path) {
            /* All flags are not present until the SD card is. */
            if (!IsSdCardInitialized()) {
                return false;
            }

            /* Mount the SD card. */
            FsFileSystem sd_fs = {};
            if (R_FAILED(fsOpenSdCardFileSystem(&sd_fs))) {
                return false;
            }
            ON_SCOPE_EXIT { serviceClose(&sd_fs.s); };

            /* Open the file. */
            FsFile flag_file;
            if (R_FAILED(fsFsOpenFile(&sd_fs, flag_path, FsOpenMode_Read, &flag_file))) {
                return false;
            }
            fsFileClose(&flag_file);

            return true;
        }

    }

    /* Flag utilities. */
    bool HasFlag(const sm::MitmProcessInfo &process_info, const char *flag) {
        return HasContentSpecificFlag(process_info.program_id, flag) || (process_info.override_status.IsHbl() && HasHblFlag(flag));
    }

    bool HasContentSpecificFlag(ncm::ProgramId program_id, const char *flag) {
        char content_flag[FS_MAX_PATH];
        std::snprintf(content_flag, sizeof(content_flag) - 1, "/atmosphere/contents/%016lx/flags/%s.flag", static_cast<u64>(program_id), flag);
        return HasFlagFile(content_flag);
    }

    bool HasGlobalFlag(const char *flag) {
        char global_flag[FS_MAX_PATH];
        std::snprintf(global_flag, sizeof(global_flag) - 1, "/atmosphere/flags/%s.flag", flag);
        return HasFlagFile(global_flag);
    }

    bool HasHblFlag(const char *flag) {
        char hbl_flag[0x100];
        std::snprintf(hbl_flag, sizeof(hbl_flag) - 1, "hbl_%s", flag);
        return HasGlobalFlag(hbl_flag);
    }

}
