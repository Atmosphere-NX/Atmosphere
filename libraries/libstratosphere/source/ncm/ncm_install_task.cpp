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
#include <stratosphere.hpp>

namespace ams::ncm {

    Result InstallTaskBase::OnPrepareComplete() {
        return ResultSuccess();
    }

    Result InstallTaskBase::GetLatestVersion(std::optional<u32> *out_version, u64 id) {
        return ncm::ResultContentMetaNotFound();
    }

    Result InstallTaskBase::CheckInstallable() {
        return ResultSuccess();
    }

    Result InstallTaskBase::OnExecuteComplete() {
        return ResultSuccess();
    }

    void InstallTaskBase::Cancel() {
        std::scoped_lock lk(this->cancel_mutex);
        this->cancel_requested = true;
    }

    void InstallTaskBase::ResetCancel() {
        std::scoped_lock lk(this->cancel_mutex);
        this->cancel_requested = false;
    }

    bool InstallTaskBase::IsCancelRequested() {
        std::scoped_lock lk(this->cancel_mutex);
        return this->cancel_requested;
    }

    Result InstallTaskBase::Initialize(StorageId install_storage, InstallTaskDataBase *data, u32 config) {
        R_UNLESS(IsInstallableStorage(install_storage), ncm::ResultUnknownStorage());
        this->install_storage = install_storage;
        this->data = data;
        this->config = config;
        return data->GetProgress(std::addressof(this->progress));
    }

    Result InstallTaskBase::Prepare() {
        /* Call the implementation. */
        Result result = this->PrepareImpl();
        
        /* Update the last result. */
        this->SetLastResult(result);
        return result;
    }

    Result InstallTaskBase::PrepareImpl() {
        /* Reset the throughput. */
        this->ResetThroughputMeasurement();

        /* Get the current progress. */
        InstallProgress progress = this->GetProgress();

        /* Transition from NotPrepared to DataPrepared. */
        if (progress.state == InstallProgressState::NotPrepared) {
            R_TRY(this->PrepareInstallContentMetaData());
            R_TRY(this->PrepareDependency());
            R_TRY(this->CheckInstallable());
            this->SetProgressState(InstallProgressState::DataPrepared);
        }

        /* Get the current progress. */
        progress = this->GetProgress();

        /* Transition from DataPrepared to Prepared. */
        if (progress.state == InstallProgressState::DataPrepared) {
            R_TRY(this->PreparePlaceHolder());
            this->SetProgressState(InstallProgressState::Prepared);
        }

        /* Signal prepare is completed. */
        return this->OnPrepareComplete();
    }

    void InstallTaskBase::SetLastResult(Result last_result) {
        std::scoped_lock lk(this->progress_mutex);
        this->data->SetLastResult(last_result);
        this->progress.SetLastResult(last_result);
    }

    Result InstallTaskBase::GetPreparedPlaceHolderPath(Path *out_path, u64 id, ContentMetaType meta_type, ContentType type) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(this->data->Count(std::addressof(count)));
        R_UNLESS(count > 0, ncm::ResultPlaceHolderNotFound());

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(this->data->Get(std::addressof(content_meta), i));
            const InstallContentMetaReader reader = content_meta.GetReader();

            /* Ensure content meta matches the key and meta type. */
            if (reader.GetKey().id != id || reader.GetKey().type != meta_type) {
                continue;
            }

            /* Attempt to obtain a content info for the content type. */
            if (const auto content_info = reader.GetContentInfo(type); content_info != nullptr) {
                /* Open the relevant content storage. */
                ContentStorage content_storage;
                R_TRY(ncm::OpenContentStorage(&content_storage, content_info->storage_id));
                
                /* Get the placeholder path. */
                /* N doesn't bother checking the result. */
                content_storage.GetPlaceHolderPath(out_path, content_info->placeholder_id);
                return ResultSuccess();
            }
        }

        return ncm::ResultPlaceHolderNotFound();
    }

    Result InstallTaskBase::CountInstallContentMetaData(s32 *out_count) {
        return this->data->Count(out_count);
    }

    Result InstallTaskBase::GetInstallContentMetaData(InstallContentMeta *out_content_meta, s32 index) {
        return this->data->Get(out_content_meta, index);
    }

    Result InstallTaskBase::CalculateRequiredSize(size_t *out_size) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(this->data->Count(std::addressof(count)));

        size_t required_size = 0;
        /* Iterate over each entry. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(this->data->Get(std::addressof(content_meta), i));
            const auto reader = content_meta.GetReader();

            /* Sum the sizes from the content infos. */
            for (size_t j = 0; j < reader.GetContentCount(); j++) {
                const auto *content_info = reader.GetContentInfo(j);

                if (content_info->install_state == InstallState::NotPrepared) {
                    required_size += ncm::CalculateRequiredSize(content_info->GetSize());
                }
            }
        }

        *out_size = required_size;
        return ResultSuccess();
    }

    void InstallTaskBase::ResetThroughputMeasurement() {
        std::scoped_lock lk(this->throughput_mutex);
        this->throughput.elapsed_time = TimeSpan();
        this->throughput_start_time = TimeSpan();
        this->throughput.installed = 0;
    }

    void InstallTaskBase::SetProgressState(InstallProgressState state) {
        std::scoped_lock(this->progress_mutex);
        this->data->SetState(state);
        this->progress.state = state;
    }

    Result InstallTaskBase::PreparePlaceHolder() {
        static os::Mutex placeholder_mutex;
        size_t total_size = 0;

        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(this->data->Count(std::addressof(count)));

        for (s32 i = 0; i < count; i++) {
            R_UNLESS(!this->IsCancelRequested(), ncm::ResultCreatePlaceHolderCancelled());
            std::scoped_lock lk(placeholder_mutex);

            InstallContentMeta content_meta;
            R_TRY(this->data->Get(&content_meta, i));

            auto writer = content_meta.GetWriter();
            StorageId storage_id = static_cast<StorageId>(writer.GetHeader()->storage_id);

            /* Automatically choose a suitable storage id. */
            if (storage_id == StorageId::None) {
                R_TRY(ncm::SelectDownloadableStorage(std::addressof(storage_id), storage_id, writer.CalculateContentRequiredSize()));
            }

            /* Update the data when we are done. */
            ON_SCOPE_EXIT { this->data->Update(content_meta, i); };

            /* Open the relevant content storage. */
            ContentStorage content_storage;
            R_TRY(ncm::OpenContentStorage(&content_storage, storage_id));

            /* Update the storage id in the header. */
            writer.SetStorageId(storage_id);
        
            for (size_t j = 0; j < writer.GetContentCount(); j++) {
                R_UNLESS(!this->IsCancelRequested(), ncm::ResultCreatePlaceHolderCancelled());
                auto *content_info = writer.GetWritableContentInfo(j);

                /* Check if we have the content already exists. */
                bool has_content;
                R_TRY(content_storage.Has(&has_content, content_info->GetId()));

                if (has_content) {
                    /* Add the size of installed content infos to the total size. */
                    if (content_info->install_state == InstallState::Installed) {
                        total_size += content_info->GetSize();
                    }

                    /* Update the install state. */
                    content_info->install_state = InstallState::AlreadyExists;
                } else {
                    if (content_info->install_state == InstallState::NotPrepared) {
                        /* Generate a placeholder id. */
                        const PlaceHolderId placeholder_id = content_storage.GeneratePlaceHolderId();
                        
                        /* Update the placeholder id in the content info. */
                        content_info->placeholder_id = placeholder_id;
                        
                        /* Create the placeholder. */
                        R_TRY(content_storage.CreatePlaceHolder(placeholder_id, content_info->GetId(), content_info->GetSize()));
                        
                        /* Update the install state. */
                        content_info->install_state = InstallState::Prepared;
                    }

                    /* Update the storage id for the content info. */
                    content_info->storage_id = storage_id;

                    /* Add the size of this content info to the total size. */
                    total_size += content_info->GetSize();
                }
            }
        }

        this->SetTotalSize(total_size);
        return ResultSuccess();
    }

    Result InstallTaskBase::Cleanup() {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(this->data->Count(std::addressof(count)));

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Get the content meta. */
            InstallContentMeta content_meta;
            R_TRY(this->data->Get(&content_meta, i));

            /* Cleanup the content meta. */
            /* N doesn't check the result of this. */
            this->CleanupOne(content_meta);
        }

        /* Cleanup the data and progress. */
        R_TRY(this->data->Cleanup());
        this->CleanupProgress();
        return ResultSuccess();
    }

    Result InstallTaskBase::CleanupOne(const InstallContentMeta &content_meta) {
        /* Obtain a reader and get the storage id. */
        const auto reader = content_meta.GetReader();
        R_SUCCEED_IF(reader.GetStorageId() == StorageId::None);

        /* Open the relevant content storage. */
        ContentStorage content_storage;
        R_TRY(ncm::OpenContentStorage(&content_storage, reader.GetStorageId()));

        /* Iterate over content infos. */
        for (size_t i = 0; i < reader.GetContentCount(); i++) {
            auto *content_info = reader.GetContentInfo(i);

            /* Delete placeholders for Prepared or Installed content infos. */
            if (content_info->install_state == InstallState::Prepared || content_info->install_state == InstallState::Installed) {
                content_storage.DeletePlaceHolder(content_info->placeholder_id);
            }
        }

        return ResultSuccess();
    }

    void InstallTaskBase::CleanupProgress() {
        std::scoped_lock(this->progress_mutex);
        this->progress.installed_size = 0;
        this->progress.total_size = 0;
        this->progress.state = InstallProgressState::NotPrepared;
        this->progress.SetLastResult(ResultSuccess());
    }

    Result InstallTaskBase::ListContentMetaKey(s32 *out_keys_written, StorageContentMetaKey *out_keys, s32 out_keys_count, s32 offset, ListContentMetaKeyFilter filter) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(this->data->Count(std::addressof(count)));

        /* Offset exceeds keys that can be written. */
        if (count <= offset) {
            *out_keys_written = 0;
            return ResultSuccess();
        }

        if (filter == ListContentMetaKeyFilter::All) {
            const size_t num_keys = std::min(count, offset + out_keys_count);

            /* Iterate over content meta. */
            for (s32 i = offset; i < num_keys; i++) {
                /* Obtain the content meta. */
                InstallContentMeta content_meta;
                R_TRY(this->data->Get(&content_meta, i));

                /* Write output StorageContentMetaKey. */
                const auto reader = content_meta.GetReader();
                StorageContentMetaKey &storage_key = out_keys[i - offset];
                storage_key.key = reader.GetKey();
                storage_key.storage_id = reader.GetStorageId();
            }

            /* Output the number of keys written. */
            *out_keys_written = num_keys - offset;
        } else {
            s32 keys_written = 0;
            s32 curr_offset = 0;

            /* Iterate over content meta. */
            for (s32 i = 0; i < count; i++) {
                /* Obtain the content meta. */
                InstallContentMeta content_meta;
                R_TRY(this->data->Get(&content_meta, i));

                /* Create a reader and check if the content has been committed. */
                const auto reader = content_meta.GetReader();
                const bool committed = reader.GetHeader()->committed;

                /* Apply filter. */
                if ((!committed && filter == ListContentMetaKeyFilter::Committed) || (committed && filter == ListContentMetaKeyFilter::NotCommitted)) {
                    continue;
                }

                /* Write output StorageContentMetaKey if at a suitable offset. */
                if (curr_offset >= offset) {
                    StorageContentMetaKey &storage_key = out_keys[keys_written++];
                    storage_key.key = reader.GetKey();
                    storage_key.storage_id = reader.GetStorageId();
                }

                /* Increment the current offset. */
                curr_offset++;

                /* We can't write any more output keys. */
                if (keys_written >= out_keys_count) {
                    break;
                }
            }

            /* Output the number of keys written. */
            *out_keys_written = keys_written;
        }

        return ResultSuccess();
    }

    Result InstallTaskBase::ListApplicationContentMetaKey(s32 *out_keys_written, ApplicationContentMetaKey *out_keys, s32 out_keys_count, s32 offset) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(this->data->Count(std::addressof(count)));

        s32 keys_written = 0;

        /* Iterate over content meta. */
        for (s32 i = offset; i < std::min(count, offset + out_keys_count); i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(this->data->Get(&content_meta, i));

            /* Create a reader. */
            const auto reader = content_meta.GetReader();

            /* Ensure this key has an application id. */
            if (!reader.GetApplicationId()) {
                continue;
            }

            /* Write output ApplicationContentMetaKey. */
            ApplicationContentMetaKey &out_key = out_keys[keys_written++];
            out_key.key = reader.GetKey();
            out_key.application_id = *reader.GetApplicationId();
        }

        *out_keys_written = keys_written;
        return ResultSuccess();
    }

    /* ... */

    void InstallTaskBase::UpdateThroughputMeasurement(s64 throughput) {
        std::scoped_lock lk(this->throughput_mutex);

        if (this->throughput_start_time.GetNanoSeconds() != 0) {
            this->throughput.installed += throughput;
            /* TODO. */
        }
    }

    bool InstallTaskBase::IsNecessaryInstallTicket(const fs::RightsId &rights_id) {
        /* If the title has no rights, there's no ticket to install. */
        fs::RightsId empty_rights_id = {};
        if (std::memcmp(std::addressof(rights_id), std::addressof(empty_rights_id), sizeof(fs::RightsId)) == 0) {
            return false;
        }

        /* TODO: Support detecting if a title requires rights. */
        /* TODO: How should es be handled without undesired effects? */
        return false;
    }

    void InstallTaskBase::SetTotalSize(s64 size) {
        std::scoped_lock(this->progress_mutex);
        this->progress.total_size = size;
    }

    /* ... */

    void InstallTaskBase::PrepareAgain() {
        this->SetProgressState(InstallProgressState::NotPrepared);
    }

    Result InstallTaskBase::PrepareDependency() {
        return ResultSuccess();
    }

    /* ... */

    InstallProgress InstallTaskBase::GetProgress() {
        std::scoped_lock lk(this->progress_mutex);
        return this->progress;
    }

    void InstallTaskBase::ResetLastResult() {
        this->SetLastResult(ResultSuccess());
    }

    s64 InstallTaskBase::GetThroughput() {
        std::scoped_lock lk(this->throughput_mutex);
        return this->throughput.installed;
    }

}
