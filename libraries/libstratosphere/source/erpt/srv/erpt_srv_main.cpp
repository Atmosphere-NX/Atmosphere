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
#include "erpt_srv_allocator.hpp"
#include "erpt_srv_context_record.hpp"
#include "erpt_srv_context.hpp"
#include "erpt_srv_reporter.hpp"
#include "erpt_srv_journal.hpp"
#include "erpt_srv_service.hpp"

namespace ams::erpt::srv {

    lmem::HeapHandle g_heap_handle;

    namespace {

        constexpr fs::SystemSaveDataId SystemSaveDataId          = 0x80000000000000D1;
        constexpr                  u32 SystemSaveDataFlags       = fs::SaveDataFlags_KeepAfterResettingSystemSaveDataWithoutUserSaveData;
        constexpr                  s64 SystemSaveDataSize        = 11_MB;
        constexpr                  s64 SystemSaveDataJournalSize = 2720_KB;

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

            return ResultSuccess();
        }

        Result MountSystemSaveData() {
            fs::DisableAutoSaveDataCreation();

            /* Extend the system save data. */
            /* NOTE: Nintendo does not check result of this. */
            ExtendSystemSaveData();

            R_TRY_CATCH(fs::MountSystemSaveData(ReportStoragePath, SystemSaveDataId)) {
                R_CATCH(fs::ResultTargetNotFound) {
                    R_TRY(fs::CreateSystemSaveData(SystemSaveDataId, SystemSaveDataSize, SystemSaveDataJournalSize, SystemSaveDataFlags));
                    R_TRY(fs::MountSystemSaveData(ReportStoragePath, SystemSaveDataId));
                }
            } R_END_TRY_CATCH;

            return ResultSuccess();
        }

    }

    Result Initialize(u8 *mem, size_t mem_size) {
        R_ABORT_UNLESS(time::Initialize());

        g_heap_handle = lmem::CreateExpHeap(mem, mem_size, lmem::CreateOption_ThreadSafe);
        AMS_ABORT_UNLESS(g_heap_handle != nullptr);

        fs::SetAllocator(Allocate, DeallocateWithSize);

        R_ABORT_UNLESS(fs::MountSdCardErrorReportDirectoryForAtmosphere(ReportOnSdStoragePath));

        R_ABORT_UNLESS(MountSystemSaveData());

        for (auto i = 0; i < CategoryId_Count; i++) {
            Context *ctx = new Context(static_cast<CategoryId>(i), 1);
            AMS_ABORT_UNLESS(ctx != nullptr);
        }

        Journal::Restore();

        return ResultSuccess();
    }

    Result InitializeAndStartService() {
        return InitializeService();
    }

    Result SetSerialNumberAndOsVersion(const char *sn, u32 sn_len, const char *os, u32 os_len, const char *os_priv, u32 os_priv_len) {
        return Reporter::SetSerialNumberAndOsVersion(sn, sn_len, os, os_len, os_priv, os_priv_len);
    }

    Result SetProductModel(const char *model, u32 model_len) {
        /* NOTE: Nintendo does not check that this allocation succeeds. */
        auto record = std::make_unique<ContextRecord>(CategoryId_ProductModelInfo);
        R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

        R_TRY(record->Add(FieldId_ProductModel, model, model_len));
        R_TRY(Context::SubmitContextRecord(std::move(record)));

        return ResultSuccess();
    }

    Result SetRegionSetting(const char *region, u32 region_len) {
        /* NOTE: Nintendo does not check that this allocation succeeds. */
        auto record = std::make_unique<ContextRecord>(CategoryId_RegionSettingInfo);
        R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

        R_TRY(record->Add(FieldId_RegionSetting, region, region_len));
        R_TRY(Context::SubmitContextRecord(std::move(record)));

        return ResultSuccess();
    }

    Result SetRedirectNewReportsToSdCard(bool redirect) {
        Reporter::SetRedirectNewReportsToSdCard(redirect);
        return ResultSuccess();
    }

    void Wait() {
        return WaitService();
    }


}
