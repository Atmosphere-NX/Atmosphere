/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
#include <stratosphere/ncm/ncm_install_task_data.hpp>

namespace ams::ncm {

/* protected:
PrepareContentMeta (both), WritePlaceHolderBuffer, Get/Delete InstallContentMetaData, PrepareDependency, PrepareSystemDependency, PrepareContentMetaIfLatest, GetConfig, WriteContentMetaToPlaceHolder, GetInstallStorage, GetSystemUpdateTaskApplyInfo, CanContinue
*/
    struct InstallThroughput {
        s64 installed;
        TimeSpan elapsed_time;
    };

    enum class ListContentMetaKeyFilter : u8 {
        All          = 0,
        Committed    = 1,
        NotCommitted = 2,
    };

    class InstallTaskBase {
        private:
            crypto::Sha256Generator sha256_generator;
            StorageId install_storage;
            InstallTaskDataBase *data;
            InstallProgress progress;
            os::Mutex progress_mutex;
            u32 config;
            os::Mutex cancel_mutex;
            bool cancel_requested;
            InstallThroughput throughput;
            TimeSpan throughput_start_time;
            os::Mutex throughput_mutex;
            /* ... */
        public:
            virtual ~InstallTaskBase() { /* TODO */ };
        private:
            Result PrepareImpl();
        protected:
            Result Initialize(StorageId install_storage, InstallTaskDataBase *data, u32 config);
            Result CountInstallContentMetaData(s32 *out_count);
            Result GetInstallContentMetaData(InstallContentMeta *out_content_meta, s32 index);

            void PrepareAgain();
        public:
            /* TODO: Fix access types. */
            bool IsCancelRequested();
            Result Prepare();
            void SetLastResult(Result last_result);
            Result GetPreparedPlaceHolderPath(Path *out_path, u64 id, ContentMetaType meta_type, ContentType type);
            Result CalculateRequiredSize(size_t *out_size);
            void ResetThroughputMeasurement();
            void SetProgressState(InstallProgressState state);

            void IncrementProgress(s64 size);
            void UpdateThroughputMeasurement(s64 throughput);
            bool IsNecessaryInstallTicket(const fs::RightsId &rights_id);
            void SetTotalSize(s64 size);
            Result PreparePlaceHolder();
            Result Cleanup();
            Result CleanupOne(const InstallContentMeta &content_meta);
            void CleanupProgress();
            Result ListContentMetaKey(s32 *out_keys_written, StorageContentMetaKey *out_keys, s32 out_keys_count, s32 offset, ListContentMetaKeyFilter filter);
            Result ListApplicationContentMetaKey(s32 *out_keys_written, ApplicationContentMetaKey *out_keys, s32 out_keys_count, s32 offset);

            void ResetLastResult();
            s64 GetThroughput();
        protected:
            virtual Result OnPrepareComplete();
            virtual Result PrepareDependency();
        public:
            /* TODO: Fix access types. */
            virtual void Cancel();
            virtual void ResetCancel();
            virtual InstallProgress GetProgress();
            virtual Result PrepareInstallContentMetaData() = 0;
            void *GetInstallContentMetaInfo;
            virtual Result GetLatestVersion(std::optional<u32> *out_version, u64 id);
            virtual Result CheckInstallable();
            virtual Result OnExecuteComplete();
            void *OnWritePlaceHolder;
            void *InstallTicket;
    };

}
