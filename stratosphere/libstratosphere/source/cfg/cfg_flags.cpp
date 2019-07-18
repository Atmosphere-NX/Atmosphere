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
#include <stratosphere/cfg.hpp>

namespace sts::cfg {

    namespace {

        /* Helper. */
        bool HasFlagFile(const char *flag_path) {
            /* All flags are not present until the SD card is. */
            if (!IsSdCardInitialized()) {
                return false;
            }

            /* Mount the SD card. */
            FsFileSystem sd_fs = {};
            if (R_FAILED(fsMountSdcard(&sd_fs))) {
                return false;
            }
            ON_SCOPE_EXIT { serviceClose(&sd_fs.s); };

            /* Open the file. */
            FsFile flag_file;
            if (R_FAILED(fsFsOpenFile(&sd_fs, flag_path, FS_OPEN_READ, &flag_file))) {
                return false;
            }
            fsFileClose(&flag_file);

            return true;
        }

    }

    /* Flag utilities. */
    bool HasFlag(ncm::TitleId title_id, const char *flag) {
        return HasTitleSpecificFlag(title_id, flag) || (IsHblTitleId(title_id) && HasHblFlag(flag));
    }

    bool HasTitleSpecificFlag(ncm::TitleId title_id, const char *flag) {
        char title_flag[FS_MAX_PATH];
        std::snprintf(title_flag, sizeof(title_flag) - 1, "/atmosphere/titles/%016lx/flags/%s.flag", static_cast<u64>(title_id), flag);
        return HasFlagFile(title_flag);
    }

    bool HasGlobalFlag(const char *flag) {
        char title_flag[FS_MAX_PATH];
        std::snprintf(title_flag, sizeof(title_flag) - 1, "/atmosphere/flags/%s.flag", flag);
        return HasFlagFile(title_flag);
    }

    bool HasHblFlag(const char *flag) {
        char hbl_flag[0x100];
        std::snprintf(hbl_flag, sizeof(hbl_flag) - 1, "hbl_%s", flag);
        return HasGlobalFlag(hbl_flag);
    }

}
