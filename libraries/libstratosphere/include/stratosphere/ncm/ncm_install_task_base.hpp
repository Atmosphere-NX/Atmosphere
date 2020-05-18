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
#include <stratosphere/ncm/ncm_install_task_occupied_size.hpp>

namespace ams::ncm {

    enum class ListContentMetaKeyFilter : u8 {
        All          = 0,
        Committed    = 1,
        NotCommitted = 2,
    };

    enum InstallConfig {
        InstallConfig_None                   = (0 << 0),
        InstallConfig_SystemUpdate           = (1 << 2),
        InstallConfig_RequiresExFatDriver    = (1 << 3),
        InstallConfig_IgnoreTicket           = (1 << 4),
    };

    struct InstallThroughput {
        s64 installed;
        TimeSpan elapsed_time;
    };

    struct InstallContentMetaInfo {
        ContentId content_id;
        s64 content_size;
        ContentMetaKey key;
        bool verify_digest;
        bool has_key;
        Digest digest;

        static constexpr InstallContentMetaInfo MakeVerifiable(const ContentId &cid, s64 sz, const ContentMetaKey &ky, const Digest &d) {
            return {
                .content_id    = cid,
                .content_size  = sz,
                .key           = ky,
                .verify_digest = true,
                .has_key       = true,
                .digest        = d,
            };
        }

        static constexpr InstallContentMetaInfo MakeUnverifiable(const ContentId &cid, s64 sz, const ContentMetaKey &ky) {
            return {
                .content_id    = cid,
                .content_size  = sz,
                .key           = ky,
                .verify_digest = false,
                .has_key       = true,
            };
        }

        static constexpr InstallContentMetaInfo MakeUnverifiable(const ContentId &cid, s64 sz) {
            return {
                .content_id    = cid,
                .content_size  = sz,
                .verify_digest = false,
                .has_key       = false,
            };
        }
    };

    static_assert(sizeof(InstallContentMetaInfo) == 0x50);

    class InstallTaskBase {
        NON_COPYABLE(InstallTaskBase);
        NON_MOVEABLE(InstallTaskBase);
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
            FirmwareVariationId firmware_variation_id;
        private:
            ALWAYS_INLINE Result SetLastResultOnFailure(Result result) {
                if (R_FAILED(result)) {
                    this->SetLastResult(result);
                }
                return result;
            }
        public:
            InstallTaskBase() : data(), progress(), progress_mutex(false), cancel_mutex(false), cancel_requested(), throughput_mutex(false) { /* ... */ }
            virtual ~InstallTaskBase() { /* ... */ };
        public:
            virtual void Cancel();
            virtual void ResetCancel();

            Result Prepare();
            Result GetPreparedPlaceHolderPath(Path *out_path, u64 id, ContentMetaType meta_type, ContentType type);
            Result CalculateRequiredSize(s64 *out_size);
            Result Cleanup();
            Result ListContentMetaKey(s32 *out_keys_written, StorageContentMetaKey *out_keys, s32 out_keys_count, s32 offset, ListContentMetaKeyFilter filter);
            Result ListContentMetaKey(s32 *out_keys_written, StorageContentMetaKey *out_keys, s32 out_keys_count, s32 offset) { return this->ListContentMetaKey(out_keys_written, out_keys, out_keys_count, offset, ListContentMetaKeyFilter::All); }
            Result ListApplicationContentMetaKey(s32 *out_keys_written, ApplicationContentMetaKey *out_keys, s32 out_keys_count, s32 offset);
            Result Execute();
            Result PrepareAndExecute();
            Result Commit(const StorageContentMetaKey *keys, s32 num_keys);
            Result Commit() { return this->Commit(nullptr, 0); }
            virtual InstallProgress GetProgress();
            void ResetLastResult();
            Result IncludesExFatDriver(bool *out);
            InstallThroughput GetThroughput();
            Result CalculateContentsSize(s64 *out_size, const ContentMetaKey &key, StorageId storage_id);
            Result ListOccupiedSize(s32 *out_written, InstallTaskOccupiedSize *out_list, s32 out_list_size, s32 offset);

            Result FindMaxRequiredApplicationVersion(u32 *out);
            Result FindMaxRequiredSystemVersion(u32 *out);
        protected:
            Result Initialize(StorageId install_storage, InstallTaskDataBase *data, u32 config);

            Result PrepareContentMeta(const InstallContentMetaInfo &meta_info, std::optional<ContentMetaKey> key, std::optional<u32> source_version);
            Result PrepareContentMeta(ContentId content_id, s64 size, ContentMetaType meta_type, AutoBuffer *buffer);
            Result WritePlaceHolderBuffer(InstallContentInfo *content_info, const void *data, size_t data_size);
            void PrepareAgain();

            Result CountInstallContentMetaData(s32 *out_count);
            Result GetInstallContentMetaData(InstallContentMeta *out_content_meta, s32 index);
            Result DeleteInstallContentMetaData(const ContentMetaKey *keys, s32 num_keys);
            virtual Result GetInstallContentMetaInfo(InstallContentMetaInfo *out_info, const ContentMetaKey &key) = 0;

            virtual Result PrepareDependency();
            Result PrepareSystemUpdateDependency();
            virtual Result PrepareContentMetaIfLatest(const ContentMetaKey &key); /* NOTE: This is not virtual in Nintendo's code. We do so to facilitate downgrades. */
            u32 GetConfig() const { return this->config; }
            Result WriteContentMetaToPlaceHolder(InstallContentInfo *out_install_content_info, ContentStorage *storage, const InstallContentMetaInfo &meta_info, std::optional<bool> is_temporary);

            StorageId GetInstallStorage() const { return this->install_storage; }

            virtual Result OnPrepareComplete() { return ResultSuccess(); }

            Result GetSystemUpdateTaskApplyInfo(SystemUpdateTaskApplyInfo *out);

            Result CanContinue();
        private:
            bool IsCancelRequested();
            Result PrepareImpl();
            Result ExecuteImpl();
            Result CommitImpl(const StorageContentMetaKey *keys, s32 num_keys);
            Result CleanupOne(const InstallContentMeta &content_meta);

            Result VerifyAllNotCommitted(const StorageContentMetaKey *keys, s32 num_keys);

            virtual Result PrepareInstallContentMetaData() = 0;
            virtual Result GetLatestVersion(std::optional<u32> *out_version, u64 id) { return ncm::ResultContentMetaNotFound(); }

            virtual Result OnExecuteComplete() { return ResultSuccess(); }

            Result WritePlaceHolder(const ContentMetaKey &key, InstallContentInfo *content_info);
            virtual Result OnWritePlaceHolder(const ContentMetaKey &key, InstallContentInfo *content_info) = 0;

            bool IsNecessaryInstallTicket(const fs::RightsId &rights_id);
            virtual Result InstallTicket(const fs::RightsId &rights_id, ContentMetaType meta_type) = 0;

            Result IsNewerThanInstalled(bool *out, const ContentMetaKey &key);
            Result PreparePlaceHolder();

            void SetProgressState(InstallProgressState state);
            void IncrementProgress(s64 size);
            void SetTotalSize(s64 size);
            void SetLastResult(Result last_result);
            void CleanupProgress();

            void ResetThroughputMeasurement();
            void StartThroughputMeasurement();
            void UpdateThroughputMeasurement(s64 throughput);

            Result GetInstallContentMetaDataFromPath(AutoBuffer *out, const Path &path, const InstallContentInfo &content_info, std::optional<u32> source_version);

            InstallContentInfo MakeInstallContentInfoFrom(const InstallContentMetaInfo &info, const PlaceHolderId &placeholder_id, std::optional<bool> is_temporary);

            Result ReadContentMetaInfoList(s32 *out_count, std::unique_ptr<ContentMetaInfo[]> *out_meta_infos, const ContentMetaKey &key);
            Result ListRightsIdsByInstallContentMeta(s32 *out_count, Span<RightsId> out_span, const InstallContentMeta &content_meta, s32 offset);
        public:
            virtual Result CheckInstallable() { return ResultSuccess(); }

            void SetFirmwareVariationId(FirmwareVariationId id) { this->firmware_variation_id = id; }
            Result ListRightsIds(s32 *out_count, Span<RightsId> out_span, const ContentMetaKey &key, s32 offset);
    };

}
