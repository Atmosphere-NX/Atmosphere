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

namespace ams::erpt::srv {

    constinit lmem::HeapHandle g_heap_handle;
    constinit ams::sf::ExpHeapAllocator g_sf_allocator = {};

    namespace {

        constexpr fs::SystemSaveDataId SystemSaveDataId          = 0x80000000000000D1;
        constexpr                  u32 SystemSaveDataFlags       = fs::SaveDataFlags_KeepAfterResettingSystemSaveDataWithoutUserSaveData;
        constexpr                  s64 SystemSaveDataSize        = 11_MB;
        constexpr                  s64 SystemSaveDataJournalSize = 2720_KB;

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
            fs::DisableAutoSaveDataCreation();

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

        R_ABORT_UNLESS(MountSystemSaveData());

        g_sf_allocator.Attach(g_heap_handle);

        for (const auto category_id : CategoryIndexToCategoryIdMap) {
            Context *ctx = new Context(category_id);
            AMS_ABORT_UNLESS(ctx != nullptr);
        }

        if (R_FAILED(Journal::Restore())) {
            /* TODO: Nintendo deletes system savedata when this fails. Should we?. */
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

    Result SetSerialNumberAndOsVersion(const char *sn, u32 sn_len, const char *os, u32 os_len, const char *os_priv, u32 os_priv_len) {
        R_RETURN(Reporter::SetSerialNumberAndOsVersion(sn, sn_len, os, os_len, os_priv, os_priv_len));
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
