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
#include <stratosphere.hpp>
#include "erpt_srv_fs_info.hpp"
#include "erpt_srv_context_record.hpp"
#include "erpt_srv_context.hpp"

namespace ams::erpt::srv {

    namespace {

        Result SubmitMmcDetailInfo() {
            /* Submit the mmc cid. */
            {
                u8 mmc_cid[fs::MmcCidSize] = {};
                if (R_SUCCEEDED(fs::GetMmcCid(mmc_cid, sizeof(mmc_cid)))) {
                    /* Clear the serial number from the cid. */
                    fs::ClearMmcCidSerialNumber(mmc_cid);

                    /* Create a record. */
                    auto record = std::make_unique<ContextRecord>(CategoryId_NANDTypeInfo, sizeof(mmc_cid));
                    R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

                    /* Add the cid. */
                    R_ABORT_UNLESS(record->Add(FieldId_NANDType, mmc_cid, sizeof(mmc_cid)));

                    /* Submit the record. */
                    R_ABORT_UNLESS(Context::SubmitContextRecord(std::move(record)));
                }
            }

            /* Submit the mmc speed mode. */
            {
                /* Create a record. */
                auto record = std::make_unique<ContextRecord>(CategoryId_NANDSpeedModeInfo, 0x20);
                R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

                /* Get the speed mode. */
                fs::MmcSpeedMode speed_mode{};
                const auto res = fs::GetMmcSpeedMode(std::addressof(speed_mode));
                if (R_SUCCEEDED(res)) {
                    const char *speed_mode_name = "None";
                    switch (speed_mode) {
                        case fs::MmcSpeedMode_Identification:
                            speed_mode_name = "Identification";
                            break;
                        case fs::MmcSpeedMode_LegacySpeed:
                            speed_mode_name = "LegacySpeed";
                            break;
                        case fs::MmcSpeedMode_HighSpeed:
                            speed_mode_name = "HighSpeed";
                            break;
                        case fs::MmcSpeedMode_Hs200:
                            speed_mode_name = "Hs200";
                            break;
                        case fs::MmcSpeedMode_Hs400:
                            speed_mode_name = "Hs400";
                            break;
                        case fs::MmcSpeedMode_Unknown:
                            speed_mode_name = "Unknown";
                            break;
                        default:
                            speed_mode_name = "UnDefined";
                            break;
                    }

                    R_ABORT_UNLESS(record->Add(FieldId_NANDSpeedMode, speed_mode_name, std::strlen(speed_mode_name)));
                } else {
                    /* Getting speed mode failed, so add the result. */
                    char res_str[0x20];
                    util::SNPrintf(res_str, sizeof(res_str), "0x%08X", res.GetValue());
                    R_ABORT_UNLESS(record->Add(FieldId_NANDSpeedMode, res_str, std::strlen(res_str)));
                }

                /* Submit the record. */
                R_ABORT_UNLESS(Context::SubmitContextRecord(std::move(record)));
            }

            /* Submit the mmc extended csd. */
            {
                u8 mmc_csd[fs::MmcExtendedCsdSize] = {};
                if (R_SUCCEEDED(fs::GetMmcExtendedCsd(mmc_csd, sizeof(mmc_csd)))) {
                    /* Create a record. */
                    auto record = std::make_unique<ContextRecord>(CategoryId_NANDExtendedCsd, 0);
                    R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

                    /* Add fields from the csd. */
                    R_ABORT_UNLESS(record->Add(FieldId_NANDPreEolInfo,            static_cast<u32>(mmc_csd[fs::MmcExtendedCsdOffsetReEolInfo])));
                    R_ABORT_UNLESS(record->Add(FieldId_NANDDeviceLifeTimeEstTypA, static_cast<u32>(mmc_csd[fs::MmcExtendedCsdOffsetDeviceLifeTimeEstTypA])));
                    R_ABORT_UNLESS(record->Add(FieldId_NANDDeviceLifeTimeEstTypB, static_cast<u32>(mmc_csd[fs::MmcExtendedCsdOffsetDeviceLifeTimeEstTypB])));

                    /* Submit the record. */
                    R_ABORT_UNLESS(Context::SubmitContextRecord(std::move(record)));
                }
            }

            /* Submit the mmc patrol count. */
            {
                u32 count = 0;
                if (R_SUCCEEDED(fs::GetMmcPatrolCount(std::addressof(count)))) {
                    /* Create a record. */
                    auto record = std::make_unique<ContextRecord>(CategoryId_NANDPatrolInfo, 0);
                    R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

                    /* Add the count. */
                    R_ABORT_UNLESS(record->Add(FieldId_NANDPatrolCount, count));

                    /* Submit the record. */
                    R_ABORT_UNLESS(Context::SubmitContextRecord(std::move(record)));
                }
            }

            R_SUCCEED();
        }

        Result SubmitMmcErrorInfo() {
            /* Get the mmc error info. */
            fs::StorageErrorInfo sei = {};
            char log_buffer[erpt::ArrayBufferSizeDefault] = {};
            size_t log_size = 0;
            if (R_SUCCEEDED(fs::GetAndClearMmcErrorInfo(std::addressof(sei), std::addressof(log_size), log_buffer, sizeof(log_buffer)))) {
                /* Submit the error info. */
                {
                    /* Create a record. */
                    auto record = std::make_unique<ContextRecord>(CategoryId_NANDErrorInfo, 0);
                    R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

                    /* Add fields. */
                    R_ABORT_UNLESS(record->Add(FieldId_NANDNumActivationFailures,         sei.num_activation_failures));
                    R_ABORT_UNLESS(record->Add(FieldId_NANDNumActivationErrorCorrections, sei.num_activation_error_corrections));
                    R_ABORT_UNLESS(record->Add(FieldId_NANDNumReadWriteFailures,          sei.num_read_write_failures));
                    R_ABORT_UNLESS(record->Add(FieldId_NANDNumReadWriteErrorCorrections,  sei.num_read_write_error_corrections));

                    /* Submit the record. */
                    R_ABORT_UNLESS(Context::SubmitContextRecord(std::move(record)));
                }

                /* If we have a log, submit it. */
                if (log_size > 0) {
                    /* Create a record. */
                    auto record = std::make_unique<ContextRecord>(CategoryId_NANDDriverLog, log_size);
                    R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

                    /* Add fields. */
                    R_ABORT_UNLESS(record->Add(FieldId_NANDErrorLog, log_buffer, log_size));

                    /* Submit the record. */
                    R_ABORT_UNLESS(Context::SubmitContextRecord(std::move(record)));
                }
            }

            R_SUCCEED();
        }

        Result SubmitSdCardDetailInfo() {
            /* Submit the sd card cid. */
            {
                u8 sd_cid[fs::SdCardCidSize] = {};
                if (R_SUCCEEDED(fs::GetSdCardCid(sd_cid, sizeof(sd_cid)))) {
                    /* Clear the serial number from the cid. */
                    fs::ClearSdCardCidSerialNumber(sd_cid);

                    /* Create a record. */
                    auto record = std::make_unique<ContextRecord>(CategoryId_MicroSDTypeInfo, sizeof(sd_cid));
                    R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

                    /* Add the cid. */
                    R_ABORT_UNLESS(record->Add(FieldId_MicroSDType, sd_cid, sizeof(sd_cid)));

                    /* Submit the record. */
                    R_ABORT_UNLESS(Context::SubmitContextRecord(std::move(record)));
                }
            }

            /* Submit the sd card speed mode. */
            {
                /* Create a record. */
                auto record = std::make_unique<ContextRecord>(CategoryId_MicroSDSpeedModeInfo, 0x20);
                R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

                /* Get the speed mode. */
                fs::SdCardSpeedMode speed_mode{};
                const auto res = fs::GetSdCardSpeedMode(std::addressof(speed_mode));
                if (R_SUCCEEDED(res)) {
                    const char *speed_mode_name = "None";
                    switch (speed_mode) {
                        case fs::SdCardSpeedMode_Identification:
                            speed_mode_name = "Identification";
                            break;
                        case fs::SdCardSpeedMode_DefaultSpeed:
                            speed_mode_name = "DefaultSpeed";
                            break;
                        case fs::SdCardSpeedMode_HighSpeed:
                            speed_mode_name = "HighSpeed";
                            break;
                        case fs::SdCardSpeedMode_Sdr12:
                            speed_mode_name = "Sdr12";
                            break;
                        case fs::SdCardSpeedMode_Sdr25:
                            speed_mode_name = "Sdr25";
                            break;
                        case fs::SdCardSpeedMode_Sdr50:
                            speed_mode_name = "Sdr50";
                            break;
                        case fs::SdCardSpeedMode_Sdr104:
                            speed_mode_name = "Sdr104";
                            break;
                        case fs::SdCardSpeedMode_Ddr50:
                            speed_mode_name = "Ddr50";
                            break;
                        case fs::SdCardSpeedMode_Unknown:
                            speed_mode_name = "Unknown";
                            break;
                        default:
                            speed_mode_name = "UnDefined";
                            break;
                    }

                    R_ABORT_UNLESS(record->Add(FieldId_MicroSDSpeedMode, speed_mode_name, std::strlen(speed_mode_name)));
                } else {
                    /* Getting speed mode failed, so add the result. */
                    char res_str[0x20];
                    util::SNPrintf(res_str, sizeof(res_str), "0x%08X", res.GetValue());
                    R_ABORT_UNLESS(record->Add(FieldId_MicroSDSpeedMode, res_str, std::strlen(res_str)));
                }

                /* Submit the record. */
                R_ABORT_UNLESS(Context::SubmitContextRecord(std::move(record)));
            }

            /* Submit the area sizes. */
            {
                s64 user_area_size = 0;
                s64 prot_area_size = 0;
                const Result res_user = fs::GetSdCardUserAreaSize(std::addressof(user_area_size));
                const Result res_prot = fs::GetSdCardProtectedAreaSize(std::addressof(prot_area_size));
                if (R_SUCCEEDED(res_user) || R_SUCCEEDED(res_prot)) {
                    /* Create a record. */
                    auto record = std::make_unique<ContextRecord>(CategoryId_SdCardSizeSpec, 0);
                    R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

                    /* Add sizes. */
                    if (R_SUCCEEDED(res_user)) {
                        R_ABORT_UNLESS(record->Add(FieldId_SdCardUserAreaSize, user_area_size));
                    }
                    if (R_SUCCEEDED(res_prot)) {
                        R_ABORT_UNLESS(record->Add(FieldId_SdCardProtectedAreaSize, prot_area_size));
                    }

                    /* Submit the record. */
                    R_ABORT_UNLESS(Context::SubmitContextRecord(std::move(record)));
                }
            }

            R_SUCCEED();
        }

        Result SubmitSdCardErrorInfo() {
            /* Get the sd card error info. */
            fs::StorageErrorInfo sei = {};
            char log_buffer[erpt::ArrayBufferSizeDefault] = {};
            size_t log_size = 0;
            if (R_SUCCEEDED(fs::GetAndClearSdCardErrorInfo(std::addressof(sei), std::addressof(log_size), log_buffer, sizeof(log_buffer)))) {
                /* Submit the error info. */
                {
                    /* Create a record. */
                    auto record = std::make_unique<ContextRecord>(CategoryId_SdCardErrorInfo, 0);
                    R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

                    /* Add fields. */
                    R_ABORT_UNLESS(record->Add(FieldId_SdCardNumActivationFailures,         sei.num_activation_failures));
                    R_ABORT_UNLESS(record->Add(FieldId_SdCardNumActivationErrorCorrections, sei.num_activation_error_corrections));
                    R_ABORT_UNLESS(record->Add(FieldId_SdCardNumReadWriteFailures,          sei.num_read_write_failures));
                    R_ABORT_UNLESS(record->Add(FieldId_SdCardNumReadWriteErrorCorrections,  sei.num_read_write_error_corrections));

                    /* Submit the record. */
                    R_ABORT_UNLESS(Context::SubmitContextRecord(std::move(record)));
                }

                /* If we have a log, submit it. */
                if (log_size > 0) {
                    /* Create a record. */
                    auto record = std::make_unique<ContextRecord>(CategoryId_SdCardDriverLog, log_size);
                    R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

                    /* Add fields. */
                    R_ABORT_UNLESS(record->Add(FieldId_SdCardErrorLog, log_buffer, log_size));

                    /* Submit the record. */
                    R_ABORT_UNLESS(Context::SubmitContextRecord(std::move(record)));
                }
            }

            R_SUCCEED();
        }

        Result SubmitGameCardDetailInfo() {
            /* Create a record. */
            auto record = std::make_unique<ContextRecord>(CategoryId_GameCardCIDInfo, fs::GameCardCidSize + fs::GameCardDeviceIdSize);
            R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

            /* Add the game card cid. */
            {
                u8 gc_cid[fs::GameCardCidSize] = {};
                if (fs::IsGameCardInserted() && R_SUCCEEDED(fs::GetGameCardCid(gc_cid, sizeof(gc_cid)))) {
                    /* Add the cid. */
                    R_ABORT_UNLESS(record->Add(FieldId_GameCardCID, gc_cid, sizeof(gc_cid)));
                }
            }

            /* Add the game card device id. */
            {
                u8 gc_device_id[fs::GameCardDeviceIdSize] = {};
                if (fs::IsGameCardInserted() && R_SUCCEEDED(fs::GetGameCardDeviceId(gc_device_id, sizeof(gc_device_id)))) {
                    /* Add the cid. */
                    R_ABORT_UNLESS(record->Add(FieldId_GameCardDeviceId, gc_device_id, sizeof(gc_device_id)));
                }
            }

            /* Submit the record. */
            R_ABORT_UNLESS(Context::SubmitContextRecord(std::move(record)));

            R_SUCCEED();
        }

        Result SubmitGameCardErrorInfo() {
            /* Get the game card error info. */
            fs::GameCardErrorReportInfo ei = {};
            if (R_SUCCEEDED(fs::GetGameCardErrorReportInfo(std::addressof(ei)))) {
                /* Create a record. */
                auto record = std::make_unique<ContextRecord>(CategoryId_GameCardErrorInfo, 0);
                R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

                /* Add fields. */
                R_ABORT_UNLESS(record->Add(FieldId_GameCardCrcErrorCount,                 static_cast<u32>(ei.game_card_crc_error_num)));
                R_ABORT_UNLESS(record->Add(FieldId_GameCardAsicCrcErrorCount,             static_cast<u32>(ei.asic_crc_error_num)));
                R_ABORT_UNLESS(record->Add(FieldId_GameCardRefreshCount,                  static_cast<u32>(ei.refresh_num)));
                R_ABORT_UNLESS(record->Add(FieldId_GameCardReadRetryCount,                static_cast<u32>(ei.retry_limit_out_num)));
                R_ABORT_UNLESS(record->Add(FieldId_GameCardTimeoutRetryErrorCount,        static_cast<u32>(ei.timeout_retry_num)));
                R_ABORT_UNLESS(record->Add(FieldId_GameCardInsertionCount,                static_cast<u32>(ei.insertion_count)));
                R_ABORT_UNLESS(record->Add(FieldId_GameCardRemovalCount,                  static_cast<u32>(ei.removal_count)));
                R_ABORT_UNLESS(record->Add(FieldId_GameCardAsicInitializeCount,           ei.initialize_count));
                R_ABORT_UNLESS(record->Add(FieldId_GameCardAsicReinitializeCount,         ei.asic_reinitialize_num));
                R_ABORT_UNLESS(record->Add(FieldId_GameCardAsicReinitializeFailureCount,  ei.asic_reinitialize_failure_num));
                R_ABORT_UNLESS(record->Add(FieldId_GameCardAsicReinitializeFailureDetail, ei.asic_reinitialize_failure_detail));
                R_ABORT_UNLESS(record->Add(FieldId_GameCardRefreshSuccessCount,           ei.refresh_succeeded_count));
                R_ABORT_UNLESS(record->Add(FieldId_GameCardAwakenCount,                   ei.awaken_count));
                R_ABORT_UNLESS(record->Add(FieldId_GameCardAwakenFailureCount,            ei.awaken_failure_num));
                R_ABORT_UNLESS(record->Add(FieldId_GameCardReadCountFromInsert,           ei.read_count_from_insert));
                R_ABORT_UNLESS(record->Add(FieldId_GameCardReadCountFromAwaken,           ei.read_count_from_awaken));
                R_ABORT_UNLESS(record->Add(FieldId_GameCardLastReadErrorPageAddress,      ei.last_read_error_page_address));
                R_ABORT_UNLESS(record->Add(FieldId_GameCardLastReadErrorPageCount,        ei.last_read_error_page_count));

                /* Submit the record. */
                R_ABORT_UNLESS(Context::SubmitContextRecord(std::move(record)));
            }

            R_SUCCEED();
        }

        Result SubmitFileSystemErrorInfo() {
            /* Get the fsp error info. */
            fs::FileSystemProxyErrorInfo ei = {};
            if (R_SUCCEEDED(fs::GetAndClearFileSystemProxyErrorInfo(std::addressof(ei)))) {
                /* Submit FsProxyErrorInfo. */
                {
                    /* Create a record. */
                    auto record = std::make_unique<ContextRecord>(CategoryId_FsProxyErrorInfo, fat::FatErrorNameMaxLength);
                    R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

                    /* Add fields. */
                    R_ABORT_UNLESS(record->Add(FieldId_FsRemountForDataCorruptCount,         ei.rom_fs_remount_for_data_corruption_count));
                    R_ABORT_UNLESS(record->Add(FieldId_FsRemountForDataCorruptRetryOutCount, ei.rom_fs_unrecoverable_data_corruption_by_remount_count));

                    R_ABORT_UNLESS(record->Add(FieldId_FatFsError,      ei.fat_fs_error.error));
                    R_ABORT_UNLESS(record->Add(FieldId_FatFsExtraError, ei.fat_fs_error.extra_error));
                    R_ABORT_UNLESS(record->Add(FieldId_FatFsErrorDrive, ei.fat_fs_error.drive_id));
                    R_ABORT_UNLESS(record->Add(FieldId_FatFsErrorName,  ei.fat_fs_error.name, fat::FatErrorNameMaxLength));

                    R_ABORT_UNLESS(record->Add(FieldId_FsRecoveredByInvalidateCacheCount, ei.rom_fs_recovered_by_invalidate_cache_count));
                    R_ABORT_UNLESS(record->Add(FieldId_FsSaveDataIndexCount,              ei.save_data_index_count));

                    R_ABORT_UNLESS(record->Add(FieldId_FatFsBisSystemFilePeakOpenCount,      ei.bis_system_fat_report_info_1.open_file_peak_count));
                    R_ABORT_UNLESS(record->Add(FieldId_FatFsBisSystemDirectoryPeakOpenCount, ei.bis_system_fat_report_info_1.open_directory_peak_count));

                    R_ABORT_UNLESS(record->Add(FieldId_FatFsBisUserFilePeakOpenCount,      ei.bis_user_fat_report_info_1.open_file_peak_count));
                    R_ABORT_UNLESS(record->Add(FieldId_FatFsBisUserDirectoryPeakOpenCount, ei.bis_user_fat_report_info_1.open_directory_peak_count));

                    R_ABORT_UNLESS(record->Add(FieldId_FatFsSdCardFilePeakOpenCount,      ei.sd_card_fat_report_info_1.open_file_peak_count));
                    R_ABORT_UNLESS(record->Add(FieldId_FatFsSdCardDirectoryPeakOpenCount, ei.sd_card_fat_report_info_1.open_directory_peak_count));

                    R_ABORT_UNLESS(record->Add(FieldId_FatFsBisSystemUniqueFileEntryPeakOpenCount,      ei.bis_system_fat_report_info_2.open_unique_file_entry_peak_count));
                    R_ABORT_UNLESS(record->Add(FieldId_FatFsBisSystemUniqueDirectoryEntryPeakOpenCount, ei.bis_system_fat_report_info_2.open_unique_directory_entry_peak_count));

                    R_ABORT_UNLESS(record->Add(FieldId_FatFsBisUserUniqueFileEntryPeakOpenCount,      ei.bis_user_fat_report_info_2.open_unique_file_entry_peak_count));
                    R_ABORT_UNLESS(record->Add(FieldId_FatFsBisUserUniqueDirectoryEntryPeakOpenCount, ei.bis_user_fat_report_info_2.open_unique_directory_entry_peak_count));

                    R_ABORT_UNLESS(record->Add(FieldId_FatFsSdCardUniqueFileEntryPeakOpenCount,      ei.sd_card_fat_report_info_2.open_unique_file_entry_peak_count));
                    R_ABORT_UNLESS(record->Add(FieldId_FatFsSdCardUniqueDirectoryEntryPeakOpenCount, ei.sd_card_fat_report_info_2.open_unique_directory_entry_peak_count));

                    /* Submit the record. */
                    R_ABORT_UNLESS(Context::SubmitContextRecord(std::move(record)));
                }

                /* Submit FsProxyErrorInfo2. */
                {
                    /* Create a record. */
                    auto record = std::make_unique<ContextRecord>(CategoryId_FsProxyErrorInfo2, 0);
                    R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

                    /* Add fields. */
                    R_ABORT_UNLESS(record->Add(FieldId_FsDeepRetryStartCount,                       ei.rom_fs_deep_retry_start_count));
                    R_ABORT_UNLESS(record->Add(FieldId_FsUnrecoverableByGameCardAccessFailedCount, ei.rom_fs_unrecoverable_by_game_card_access_failed_count));

                    R_ABORT_UNLESS(record->Add(FieldId_FatFsBisSystemFatSafeControlResult, static_cast<u8>(ei.bis_system_fat_safe_info.result)));
                    R_ABORT_UNLESS(record->Add(FieldId_FatFsBisSystemFatErrorNumber,       ei.bis_system_fat_safe_info.error_number));
                    R_ABORT_UNLESS(record->Add(FieldId_FatFsBisSystemFatSafeErrorNumber,   ei.bis_system_fat_safe_info.safe_error_number));

                    R_ABORT_UNLESS(record->Add(FieldId_FatFsBisUserFatSafeControlResult, static_cast<u8>(ei.bis_user_fat_safe_info.result)));
                    R_ABORT_UNLESS(record->Add(FieldId_FatFsBisUserFatErrorNumber,       ei.bis_user_fat_safe_info.error_number));
                    R_ABORT_UNLESS(record->Add(FieldId_FatFsBisUserFatSafeErrorNumber,   ei.bis_user_fat_safe_info.safe_error_number));

                    /* Submit the record. */
                    R_ABORT_UNLESS(Context::SubmitContextRecord(std::move(record)));
                }
            }

            R_SUCCEED();
        }

        Result SubmitMemoryReportInfo() {
            /* Get the memory report info. */
            fs::MemoryReportInfo mri = {};
            if (R_SUCCEEDED(fs::GetAndClearMemoryReportInfo(std::addressof(mri)))) {
                /* Create a record. */
                auto record = std::make_unique<ContextRecord>(CategoryId_FsMemoryInfo, 0);
                R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

                /* Add fields. */
                R_ABORT_UNLESS(record->Add(FieldId_FsPooledBufferPeakFreeSize,          mri.pooled_buffer_peak_free_size));
                R_ABORT_UNLESS(record->Add(FieldId_FsPooledBufferRetriedCount,          mri.pooled_buffer_retried_count));
                R_ABORT_UNLESS(record->Add(FieldId_FsPooledBufferReduceAllocationCount, mri.pooled_buffer_reduce_allocation_count));

                R_ABORT_UNLESS(record->Add(FieldId_FsBufferManagerPeakFreeSize, mri.buffer_manager_peak_free_size));
                R_ABORT_UNLESS(record->Add(FieldId_FsBufferManagerRetriedCount, mri.buffer_manager_retried_count));

                R_ABORT_UNLESS(record->Add(FieldId_FsExpHeapPeakFreeSize, mri.exp_heap_peak_free_size));

                R_ABORT_UNLESS(record->Add(FieldId_FsBufferPoolPeakFreeSize, mri.buffer_pool_peak_free_size));

                R_ABORT_UNLESS(record->Add(FieldId_FsPatrolReadAllocateBufferSuccessCount, mri.patrol_read_allocate_buffer_success_count));
                R_ABORT_UNLESS(record->Add(FieldId_FsPatrolReadAllocateBufferFailureCount, mri.patrol_read_allocate_buffer_failure_count));

                R_ABORT_UNLESS(record->Add(FieldId_FsBufferManagerPeakTotalAllocatableSize, mri.buffer_manager_peak_total_allocatable_size));

                R_ABORT_UNLESS(record->Add(FieldId_FsBufferPoolMaxAllocateSize, mri.buffer_pool_max_allocate_size));

                R_ABORT_UNLESS(record->Add(FieldId_FsPooledBufferFailedIdealAllocationCountOnAsyncAccess, mri.pooled_buffer_failed_ideal_allocation_count_on_async_access));

                /* Submit the record. */
                R_ABORT_UNLESS(Context::SubmitContextRecord(std::move(record)));
            }

            R_SUCCEED();
        }

    }

    Result SubmitFsInfo() {
        /* Temporarily disable auto-abort. */
        fs::ScopedAutoAbortDisabler aad;

        /* Submit various FS info. */
        R_TRY(SubmitMmcDetailInfo());
        R_TRY(SubmitMmcErrorInfo());
        R_TRY(SubmitSdCardDetailInfo());
        R_TRY(SubmitSdCardErrorInfo());
        R_TRY(SubmitGameCardDetailInfo());
        R_TRY(SubmitGameCardErrorInfo());
        R_TRY(SubmitFileSystemErrorInfo());
        R_TRY(SubmitMemoryReportInfo());

        R_SUCCEED();
    }

}
