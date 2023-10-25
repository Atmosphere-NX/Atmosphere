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
            /* TODO */
            R_SUCCEED();
        }

        Result SubmitSdCardDetailInfo() {
            /* TODO */
            R_SUCCEED();
        }

        Result SubmitSdCardErrorInfo() {
            /* TODO */
            R_SUCCEED();
        }

        Result SubmitGameCardDetailInfo() {
            /* TODO */
            R_SUCCEED();
        }

        Result SubmitGameCardErrorInfo() {
            /* TODO */
            R_SUCCEED();
        }

        Result SubmitFileSystemErrorInfo() {
            /* TODO */
            R_SUCCEED();
        }

        Result SubmitMemoryReportInfo() {
            /* TODO */
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
