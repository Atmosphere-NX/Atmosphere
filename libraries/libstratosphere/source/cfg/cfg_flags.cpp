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

namespace ams::cfg {

    namespace {

        std::atomic<u32> g_flag_mount_count;

        /* Helper. */
        void GetFlagMountName(char *dst) {
            std::snprintf(dst, fs::MountNameLengthMax + 1, "#flag%08x", g_flag_mount_count.fetch_add(1));
        }

        bool HasFlagFile(const char *flag_path) {
            /* All flags are not present until the SD card is. */
            if (!IsSdCardInitialized()) {
                return false;
            }

            /* Mount the SD card. */
            char mount_name[fs::MountNameLengthMax + 1];
            GetFlagMountName(mount_name);
            if (R_FAILED(fs::MountSdCard(mount_name))) {
                return false;
            }
            ON_SCOPE_EXIT { fs::Unmount(mount_name); };

            /* Check if the entry exists. */
            char full_path[fs::EntryNameLengthMax + 1];
            std::snprintf(full_path, sizeof(full_path), "%s:/%s", mount_name, flag_path[0] == '/' ? flag_path + 1 : flag_path);

            bool has_file;
            if (R_FAILED(fs::HasFile(std::addressof(has_file), full_path))) {
                return false;
            }

            return has_file;
        }

    }

    /* Flag utilities. */
    bool HasFlag(const sm::MitmProcessInfo &process_info, const char *flag) {
        return HasContentSpecificFlag(process_info.program_id, flag) || (process_info.override_status.IsHbl() && HasHblFlag(flag));
    }

    bool HasContentSpecificFlag(ncm::ProgramId program_id, const char *flag) {
        char content_flag[fs::EntryNameLengthMax + 1];
        std::snprintf(content_flag, sizeof(content_flag) - 1, "/atmosphere/contents/%016lx/flags/%s.flag", static_cast<u64>(program_id), flag);
        return HasFlagFile(content_flag);
    }

    bool HasGlobalFlag(const char *flag) {
        char global_flag[fs::EntryNameLengthMax + 1];
        std::snprintf(global_flag, sizeof(global_flag) - 1, "/atmosphere/flags/%s.flag", flag);
        return HasFlagFile(global_flag);
    }

    bool HasHblFlag(const char *flag) {
        char hbl_flag[0x100];
        std::snprintf(hbl_flag, sizeof(hbl_flag) - 1, "hbl_%s", flag);
        return HasGlobalFlag(hbl_flag);
    }

}
