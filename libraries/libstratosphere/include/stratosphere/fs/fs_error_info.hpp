/*
 * Copyright (c) Atmosphère-NX
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
#pragma once
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fat/fat_file_system.hpp>

namespace ams::fs {

    struct StorageErrorInfo {
        u32 num_activation_failures;
        u32 num_activation_error_corrections;
        u32 num_read_write_failures;
        u32 num_read_write_error_corrections;
    };
    static_assert(sizeof(StorageErrorInfo) == 0x10);
    static_assert(util::is_pod<StorageErrorInfo>::value);

    struct FileSystemProxyErrorInfo {
        u32 rom_fs_remount_for_data_corruption_count;
        u32 rom_fs_unrecoverable_data_corruption_by_remount_count;
        fat::FatError fat_fs_error;
        u32 rom_fs_recovered_by_invalidate_cache_count;
        u32 save_data_index_count;
        fat::FatReportInfo1 bis_system_fat_report_info_1;
        fat::FatReportInfo1 bis_user_fat_report_info_1;
        fat::FatReportInfo1 sd_card_fat_report_info_1;
        fat::FatReportInfo2 bis_system_fat_report_info_2;
        fat::FatReportInfo2 bis_user_fat_report_info_2;
        fat::FatReportInfo2 sd_card_fat_report_info_2;
        u32 rom_fs_deep_retry_start_count;
        u32 rom_fs_unrecoverable_by_game_card_access_failed_count;
        fat::FatSafeInfo bis_system_fat_safe_info;
        fat::FatSafeInfo bis_user_fat_safe_info;

        u8 reserved[0x18];
    };
    static_assert(sizeof(FileSystemProxyErrorInfo) == 0x80);
    static_assert(util::is_pod<FileSystemProxyErrorInfo>::value);

    Result GetAndClearMmcErrorInfo(StorageErrorInfo *out_sei, size_t *out_log_size, char *out_log_buffer, size_t log_buffer_size);
    Result GetAndClearSdCardErrorInfo(StorageErrorInfo *out_sei, size_t *out_log_size, char *out_log_buffer, size_t log_buffer_size);

    Result GetAndClearFileSystemProxyErrorInfo(FileSystemProxyErrorInfo *out);

}
