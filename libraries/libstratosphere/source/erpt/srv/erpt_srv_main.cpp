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
#include "erpt_srv_allocator.hpp"
#include "erpt_srv_context_record.hpp"
#include "erpt_srv_context.hpp"
#include "erpt_srv_reporter.hpp"
#include "erpt_srv_journal.hpp"
#include "erpt_srv_service.hpp"
#include "erpt_srv_forced_shutdown.hpp"
#include "erpt_srv_notifiable_errors.hpp"

namespace ams::erpt::srv {

    constinit lmem::HeapHandle g_heap_handle;
    constinit ams::sf::ExpHeapAllocator g_sf_allocator = {};

    namespace {

        constexpr fs::SystemSaveDataId SystemSaveDataId          = 0x80000000000000D1;
        constexpr                  u32 SystemSaveDataFlags       = fs::SaveDataFlags_KeepAfterResettingSystemSaveDataWithoutUserSaveData;
        constexpr                  s64 SystemSaveDataSize        = 11_MB;
        constexpr                  s64 SystemSaveDataJournalSize = 2720_KB;
        constexpr                  u32 DefaultThrottleTimeWindowSeconds = 3;

        constinit bool g_automatic_report_cleanup_enabled = true;

        Result ExtendSystemSaveData() {
            s64 cur_journal_size;
            s64 cur_savedata_size;

            R_TRY(fs::GetSaveDataJournalSize(std::addressof(cur_journal_size),    SystemSaveDataId));
            R_TRY(fs::GetSaveDataAvailableSize(std::addressof(cur_savedata_size), SystemSaveDataId));

            if (cur_journal_size < SystemSaveDataJournalSize || cur_savedata_size < SystemSaveDataSize) {
                if (hos::GetVersion() >= hos::Version_3_0_0) {
                    R_TRY(fs::ExtendSaveData(fs::SaveDataSpaceId::System, SystemSaveDataId, SystemSaveDataSize, SystemSaveDataJournalSize));
                }
            }

            R_SUCCEED();
        }

        Result MountSystemSaveData() {
            if (hos::GetVersion() < hos::Version_21_0_0) {
                fs::DisableAutoSaveDataCreation();
            }

            /* Extend the system save data. */
            /* NOTE: Nintendo used to not check the result of this; they do now, but . */
            static_cast<void>(ExtendSystemSaveData());

            R_TRY_CATCH(fs::MountSystemSaveData(ReportStoragePath, SystemSaveDataId)) {
                R_CATCH(fs::ResultTargetNotFound) {
                    R_TRY(fs::CreateSystemSaveData(SystemSaveDataId, SystemSaveDataSize, SystemSaveDataJournalSize, SystemSaveDataFlags));
                    R_TRY(fs::MountSystemSaveData(ReportStoragePath, SystemSaveDataId));
                }
            } R_END_TRY_CATCH;

            R_SUCCEED();
        }

    }

    namespace {

        int MakeProductModelString(char *dst, size_t dst_size, settings::system::ProductModel model) {
            switch (model) {
                case settings::system::ProductModel_Invalid: return util::Strlcpy(dst, "Invalid", static_cast<int>(dst_size));
                case settings::system::ProductModel_Nx:      return util::Strlcpy(dst, "NX", static_cast<int>(dst_size));
                default:                                     return util::SNPrintf(dst, dst_size, "%d", static_cast<int>(model));
            }
        }

        const char *GetRegionString(settings::system::RegionCode code) {
            switch (code) {
                case settings::system::RegionCode_Japan:               return "Japan";
                case settings::system::RegionCode_Usa:                 return "Usa";
                case settings::system::RegionCode_Europe:              return "Europe";
                case settings::system::RegionCode_Australia:           return "Australia";
                case settings::system::RegionCode_HongKongTaiwanKorea: return "HongKongTaiwanKorea";
                case settings::system::RegionCode_China:               return "China";
                default:                                               return "RegionUnknown";
            }
        }

    }

    const erpt::SystemInfo &GetSystemInfo() {
        static const erpt::SystemInfo s_info = [] {
            erpt::SystemInfo info = {};

            settings::system::FirmwareVersion firmware_version = {};
            settings::system::GetFirmwareVersion(std::addressof(firmware_version));

            util::Strlcpy(info.os_version, firmware_version.display_version, sizeof(info.os_version));

            const auto os_priv_len = util::SNPrintf(info.private_os_version, sizeof(info.private_os_version), "%s (%.8s)", firmware_version.display_name, firmware_version.revision);
            AMS_ASSERT(static_cast<size_t>(os_priv_len) < sizeof(info.private_os_version));
            AMS_UNUSED(os_priv_len);

            const auto pm_len = MakeProductModelString(info.product_model, sizeof(info.product_model), settings::system::GetProductModel());
            AMS_ASSERT(static_cast<size_t>(pm_len) < sizeof(info.product_model));
            AMS_UNUSED(pm_len);

            settings::system::RegionCode region_code;
            settings::system::GetRegionCode(std::addressof(region_code));
            info.region = GetRegionString(region_code);

            return info;
        }();
        return s_info;
    }

    u32 GetThrottleTimeWindowSecondsImpl() {
        u32 seconds = DefaultThrottleTimeWindowSeconds;    
        if (settings::fwdbg::GetSettingsItemValue(std::addressof(seconds), sizeof(seconds), "erpt", "throttle_time_window_seconds") != sizeof(seconds)) {
            return DefaultThrottleTimeWindowSeconds;
        }
        return seconds;
    }

    void SetReportThrottleTimeSpan() {
        u32 seconds = GetThrottleTimeWindowSecondsImpl();
        
        const TimeSpan time_span = TimeSpan::FromSeconds(static_cast<s64>(seconds));

        Reporter::SetThrottleTimeSpan(time_span);
    }

    Result Initialize(u8 *mem, size_t mem_size) {
        R_ABORT_UNLESS(time::Initialize());

        g_heap_handle = lmem::CreateExpHeap(mem, mem_size, lmem::CreateOption_ThreadSafe);
        AMS_ABORT_UNLESS(g_heap_handle != nullptr);

        fs::InitializeForSystem();
        fs::SetAllocator(Allocate, DeallocateWithSize);
        fs::SetEnabledAutoAbort(false);

        R_ABORT_UNLESS(fs::MountSdCardErrorReportDirectoryForAtmosphere(ReportOnSdStoragePath));

        if (g_automatic_report_cleanup_enabled) {
            constexpr s64 MinimumReportCountForCleanup = 1000;
            s64 report_count = MinimumReportCountForCleanup;

            fs::DirectoryHandle dir;
            if (R_SUCCEEDED(fs::OpenDirectory(std::addressof(dir), ReportOnSdStorageRootDirectoryPath, fs::OpenDirectoryMode_All))) {
                ON_SCOPE_EXIT { fs::CloseDirectory(dir); };

                if (R_FAILED(fs::GetDirectoryEntryCount(std::addressof(report_count), dir))) {
                    report_count = MinimumReportCountForCleanup;
                }
            }

            if (report_count >= MinimumReportCountForCleanup) {
                static_cast<void>(fs::CleanDirectoryRecursively(ReportOnSdStorageRootDirectoryPath));
            }
        }

        if (hos::GetVersion() >= hos::Version_22_0_0) {
            SetReportThrottleTimeSpan();
        }

        R_ABORT_UNLESS(MountSystemSaveData());

        g_sf_allocator.Attach(g_heap_handle);

        for (const auto category_id : CategoryIndexToCategoryIdMap) {
            Context *ctx = new Context(category_id);
            AMS_ABORT_UNLESS(ctx != nullptr);
        }

        if (hos::GetVersion() >= hos::Version_21_0_0) {
            /* >= 21.0.0, Nintendo checks the result of restore and deletes the save data if it fails. */
            if (R_FAILED(Journal::Restore())) {
                /* Delete and recreate the system save data. */
                fs::Unmount(ReportStoragePath);
                R_ABORT_UNLESS(fs::DeleteSystemSaveData(fs::SaveDataSpaceId::System, SystemSaveDataId, fs::InvalidUserId));
    
                R_ABORT_UNLESS(MountSystemSaveData());
            }
        } else{
            /* Pre 21.0.0, Nintendo just calls restore and ignores the result. */
            Journal::Restore();
        }

        Reporter::UpdatePowerOnTime();
        Reporter::UpdateAwakeTime();

        R_SUCCEED();
    }

    Result InitializeAndStartService() {
        /* Initialize forced shutdown detection. */
        /* NOTE: Nintendo does not check error code here. */
        InitializeForcedShutdownDetection();

        R_RETURN(InitializeService());
    }

    Result SetSerialNumber(const char *sn, u32 sn_len) {
        R_RETURN(Reporter::SetSerialNumber(sn, sn_len));
    }

    Result SetProductModel(const char *model, u32 model_len) {
        /* NOTE: Nintendo does not check that this allocation succeeds. */
        auto record = std::make_unique<ContextRecord>(CategoryId_ProductModelInfo);
        R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

        R_TRY(record->Add(FieldId_ProductModel, model, model_len));
        R_TRY(Context::SubmitContextRecord(std::move(record)));

        R_SUCCEED();
    }

    Result SetRegionSetting(const char *region, u32 region_len) {
        /* NOTE: Nintendo does not check that this allocation succeeds. */
        auto record = std::make_unique<ContextRecord>(CategoryId_RegionSettingInfo);
        R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

        R_TRY(record->Add(FieldId_RegionSetting, region, region_len));
        R_TRY(Context::SubmitContextRecord(std::move(record)));

        R_SUCCEED();
    }

    Result SetRedirectNewReportsToSdCard(bool redirect) {
        Reporter::SetRedirectNewReportsToSdCard(redirect);
        R_SUCCEED();
    }

    Result SetEnabledAutomaticReportCleanup(bool en) {
        g_automatic_report_cleanup_enabled = en;
        R_SUCCEED();
    }

    void Wait() {
        /* Get the update event. */
        os::Event *event = GetForcedShutdownUpdateEvent();

        /* Forever wait, saving any updates. */
        while (true) {
            event->Wait();
            event->Clear();
            SaveForcedShutdownContext();
        }
    }


}
