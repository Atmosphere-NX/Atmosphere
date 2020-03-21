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

    namespace {

        bool Contains(const StorageContentMetaKey *keys, s32 num_keys, const ContentMetaKey &key, StorageId storage_id) {
            for (s32 i = 0; i < num_keys; i++) {
                const StorageContentMetaKey &storage_key = keys[i];

                /* Check if the key matches the input key and storage id. */
                if (storage_key.key == key && storage_key.storage_id == storage_id) {
                    return true;
                }
            }

            return false;
        }

        bool Contains(const ContentMetaKey *keys, s32 num_keys, const ContentMetaKey &key) {
            for (s32 i = 0; i < num_keys; i++) {
                /* Check if the key matches the input key. */
                if (keys[i] == key) {
                    return true;
                }
            }

            return false;
        }

    }

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
        R_TRY(this->SetLastResultOnFailure(this->PrepareImpl()));
        return ResultSuccess();
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

    Result InstallTaskBase::Execute() {
        R_TRY(this->SetLastResultOnFailure(this->ExecuteImpl()));
        return ResultSuccess();
    }

    Result InstallTaskBase::ExecuteImpl() {
        this->StartThroughputMeasurement();

        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(this->data->Count(std::addressof(count)));

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(this->data->Get(&content_meta, i));

            /* Update the data when we are done. */
            ON_SCOPE_EXIT { this->data->Update(content_meta, i); };

            /* Create a writer. */
            const auto writer = content_meta.GetWriter();

            /* Iterate over content infos. */
            for (size_t j = 0; j < writer.GetContentCount(); j++) {
                auto *content_info = writer.GetWritableContentInfo(j);

                /* Write prepared content infos. */
                if (content_info->install_state == InstallState::Prepared) {
                    R_TRY(this->WritePlaceHolder(writer.GetKey(), content_info));
                    content_info->install_state = InstallState::Installed;
                }
            }
        }

        /* Execution has finished, signal this and update the state. */
        R_TRY(this->OnExecuteComplete());
        this->SetProgressState(InstallProgressState::Downloaded);
        return ResultSuccess();
    }

    void InstallTaskBase::StartThroughputMeasurement() {
        std::scoped_lock lk(this->throughput_mutex);
        this->throughput.installed = 0;
        this->throughput.elapsed_time = TimeSpan();
        this->throughput_start_time = os::ConvertToTimeSpan(os::GetSystemTick());
    }

    Result InstallTaskBase::WritePlaceHolder(const ContentMetaKey &key, InstallContentInfo *content_info) {
        if (content_info->is_sha256_calculated) {
            /* Update the hash with the buffered data. */
            this->sha256_generator.InitializeWithContext(std::addressof(content_info->context));
            this->sha256_generator.Update(content_info->buffered_data, content_info->buffered_data_size);
        } else {
            /* Initialize the generator. */
            this->sha256_generator.Initialize();
        }

        {
            ON_SCOPE_EXIT {
                /* Update this content info's sha256 data. */
                this->sha256_generator.GetContext(std::addressof(content_info->context));
                content_info->buffered_data_size = this->sha256_generator.GetBufferedDataSize();
                this->sha256_generator.GetBufferedData(content_info->buffered_data, this->sha256_generator.GetBufferedDataSize());
                content_info->is_sha256_calculated = true;
            };

            /* Perform the placeholder write. */
            R_TRY(this->OnWritePlaceHolder(key, content_info));
        }

        /* Compare generated hash to expected hash if verification required. */
        if (content_info->verify_digest) {
            u8 hash[crypto::Sha256Generator::HashSize];
            this->sha256_generator.GetHash(hash, crypto::Sha256Generator::HashSize);
            R_UNLESS(std::memcmp(hash, content_info->digest.data, crypto::Sha256Generator::HashSize) == 0, ncm::ResultInvalidContentHash());
        }

        if (!(this->config & InstallConfig_IgnoreTicket)) {
            ncm::RightsId rights_id;
            {
                /* Open the content storage and obtain the rights id. */
                ncm::ContentStorage storage;
                R_TRY(OpenContentStorage(std::addressof(storage), content_info->storage_id));
                R_TRY(storage.GetRightsId(std::addressof(rights_id), content_info->placeholder_id));
            }

            /* Install a ticket if necessary. */
            if (this->IsNecessaryInstallTicket(rights_id.id)) {
                R_TRY_CATCH(this->InstallTicket(rights_id.id, content_info->meta_type)) {
                    R_CATCH(ncm::ResultIgnorableInstallTicketFailure) { /* We can ignore the installation failure. */ }
                } R_END_TRY_CATCH;
            }
        }

        return ResultSuccess();
    }

    Result InstallTaskBase::PrepareAndExecute() {
        R_TRY(this->SetLastResultOnFailure(this->PrepareImpl()));
        R_TRY(this->SetLastResultOnFailure(this->ExecuteImpl()));
        return ResultSuccess();
    }

    Result InstallTaskBase::VerifyAllNotCommitted(const StorageContentMetaKey *keys, s32 num_keys) {
        /* No keys to check. */
        R_SUCCEED_IF(keys == nullptr);
        
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(this->data->Count(std::addressof(count)));

        s32 num_not_committed = 0;

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(this->data->Get(&content_meta, i));

            /* Create a reader. */
            const auto reader = content_meta.GetReader();

            if (Contains(keys, num_keys, reader.GetKey(), reader.GetStorageId())) {
                /* Ensure content meta isn't committed. */
                R_UNLESS(!reader.GetHeader()->committed, ncm::ResultListPartiallyNotCommitted());
                num_not_committed++;
            }
        }

        /* Ensure number of uncommitted keys equals the number of input keys. */
        R_UNLESS(num_not_committed == num_keys, ncm::ResultListPartiallyNotCommitted());
        return ResultSuccess();
    }

    Result InstallTaskBase::CommitImpl(const StorageContentMetaKey *keys, s32 num_keys) {
        /* Ensure progress state is Downloaded. */
        R_UNLESS(this->GetProgress().state == InstallProgressState::Downloaded, ncm::ResultInvalidInstallTaskState());

        /* Ensure keys aren't committed. */
        R_TRY(this->VerifyAllNotCommitted(keys, num_keys));

        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(this->data->Count(std::addressof(count)));

        /* List of storages to commit. */
        StorageList commit_list;

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(this->data->Get(&content_meta, i));

            /* Create a reader. */
            const auto reader         = content_meta.GetReader();
            const auto cur_key        = reader.GetKey();
            const auto storage_id     = reader.GetStorageId();
            const size_t convert_size = reader.CalculateConvertSize();

            /* Skip content meta not contained in input keys. */
            if (keys != nullptr && !Contains(keys, num_keys, cur_key, storage_id)) {
                continue;
            }

            /* Skip already committed. This check is primarily for if keys is nullptr. */
            if (reader.GetHeader()->committed) {
                continue;
            }

            /* Helper for performing an update. */
            const auto DoUpdate = [&]() ALWAYS_INLINE_LAMBDA { return this->data->Update(content_meta, i); };
            
            /* Commit the current meta. */
            {
                /* Ensure that if something goes wrong during commit, we still try to update. */
                auto update_guard = SCOPE_GUARD { DoUpdate(); };
                
                /* Open a writer. */
                const auto writer = content_meta.GetWriter();

                /* Convert to content meta and store to a buffer. */
                std::unique_ptr<char[]> content_meta_buffer(new (std::nothrow) char[convert_size]);
                R_UNLESS(content_meta_buffer != nullptr, ncm::ResultAllocationFailed());
                reader.ConvertToContentMeta(content_meta_buffer.get(), convert_size);

                /* Open the content storage for this meta. */
                ContentStorage content_storage;
                R_TRY(OpenContentStorage(&content_storage, storage_id));

                /* Open the content meta database for this meta. */
                ContentMetaDatabase meta_db;
                R_TRY(OpenContentMetaDatabase(std::addressof(meta_db), storage_id));

                /* Iterate over content infos. */
                for (size_t j = 0; j < reader.GetContentCount(); j++) {
                    const auto *content_info = reader.GetContentInfo(j);

                    /* Register non-existing content infos. */
                    if (content_info->install_state != InstallState::AlreadyExists) {
                        R_TRY(content_storage.Register(content_info->placeholder_id, content_info->info.content_id));
                    }
                }

                /* Store the content meta. */
                R_TRY(meta_db.Set(reader.GetKey(), content_meta_buffer.get(), convert_size));

                /* Mark as committed. */
                writer.GetWritableHeader()->committed = true;

                /* Mark storage id to be committed later. */
                commit_list.Push(reader.GetStorageId());
                
                /* We successfully commited this meta, so we want to check for errors when updating. */
                update_guard.Cancel();
            }
            
            /* Try to update, checking for failure. */
            R_TRY(DoUpdate());
        }

        /* Commit all applicable content meta databases. */
        for (s32 i = 0; i < commit_list.Count(); i++) {
            ContentMetaDatabase meta_db;
            R_TRY(OpenContentMetaDatabase(std::addressof(meta_db), commit_list[i]));
            R_TRY(meta_db.Commit());
        }

        /* Change progress state to committed if keys are nullptr. */
        if (keys == nullptr) {
            this->SetProgressState(InstallProgressState::Committed);
        }

        return ResultSuccess();
    }

    Result InstallTaskBase::Commit(const StorageContentMetaKey *keys, s32 num_keys) {
        auto fatal_guard = SCOPE_GUARD { SetProgressState(InstallProgressState::Fatal); };
        R_TRY(this->SetLastResultOnFailure(this->CommitImpl(keys, num_keys)));
        fatal_guard.Cancel();
        return ResultSuccess();
    }

    Result InstallTaskBase::IncludesExFatDriver(bool *out) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(this->data->Count(std::addressof(count)));

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(this->data->Get(&content_meta, i));

            /* Check if the attributes are set for including the exfat driver. */
            if (content_meta.GetReader().GetHeader()->attributes & ContentMetaAttribute_IncludesExFatDriver) {
                *out = true;
                return ResultSuccess();
            }
        }

        *out = false;
        return ResultSuccess();
    }

    Result InstallTaskBase::WritePlaceHolderBuffer(InstallContentInfo *content_info, const void *data, size_t data_size) {
        R_UNLESS(!this->IsCancelRequested(), ncm::ResultWritePlaceHolderCancelled());

        /* Open the content storage for the content info. */
        ContentStorage content_storage;
        R_TRY(OpenContentStorage(&content_storage, content_info->storage_id));

        /* Write data to the placeholder. */
        content_storage.WritePlaceHolder(content_info->placeholder_id, content_info->written, data, data_size);
        content_info->written += data_size;

        /* Update progress/throughput if content info isn't temporary. */
        if (!content_info->is_temporary) {
            this->IncrementProgress(data_size);
            this->UpdateThroughputMeasurement(data_size);
        }

        /* Update the hash for the new data. */
        this->sha256_generator.Update(data, data_size);
        return ResultSuccess();
    }

    Result InstallTaskBase::WriteContentMetaToPlaceHolder(InstallContentInfo *out_install_content_info, ContentStorage *storage, const InstallContentMetaInfo &meta_info, std::optional<bool> is_temporary) {
        /* Generate a placeholder id. */
        auto placeholder_id = storage->GeneratePlaceHolderId();

        /* Create the placeholder. */
        R_TRY(storage->CreatePlaceHolder(placeholder_id, meta_info.content_id, meta_info.content_size));
        auto placeholder_guard = SCOPE_GUARD { storage->DeletePlaceHolder(placeholder_id); };

        /* Output install content info. */
        *out_install_content_info = this->MakeInstallContentInfoFrom(meta_info, placeholder_id, is_temporary);

        /* Write install content info. */
        R_TRY(this->WritePlaceHolder(meta_info.key, out_install_content_info));
        
        /* Don't delete the placeholder. Set state to installed. */
        placeholder_guard.Cancel();
        out_install_content_info->install_state = InstallState::Installed;
        return ResultSuccess();
    }

    InstallContentInfo InstallTaskBase::MakeInstallContentInfoFrom(const InstallContentMetaInfo &info, const PlaceHolderId &placeholder_id, std::optional<bool> is_tmp) {
        return {
            .digest         = info.digest,
            .info           = ContentInfo::Make(info.content_id, info.content_size, ContentType::Meta, 0),
            .placeholder_id = placeholder_id,
            .meta_type      = info.key.type,
            .install_state  = InstallState::Prepared,
            .verify_digest  = info.verify_digest,
            .storage_id     = StorageId::BuiltInSystem,
            .is_temporary   = is_tmp ? *is_tmp : (this->install_storage != StorageId::BuiltInSystem),
        };
    }

    /* ... */

    Result InstallTaskBase::PrepareContentMeta(ContentId content_id, s64 size, ContentMetaType meta_type, AutoBuffer *buffer) {
        /* Create a reader. */
        PackagedContentMetaReader reader(buffer->Get(), buffer->GetSize());

        /* Initialize the temporary buffer. */
        AutoBuffer tmp_buffer;
        R_TRY(tmp_buffer.Initialize(reader.CalculateConvertInstallContentMetaSize()));

        /* Convert packaged content meta to install content meta. */
        reader.ConvertToInstallContentMeta(tmp_buffer.Get(), tmp_buffer.GetSize(), InstallContentInfo::Make(ContentInfo::Make(content_id, size, ContentType::Meta), meta_type));
        
        /* Push the content meta. */
        this->data->Push(tmp_buffer.Get(), tmp_buffer.GetSize());
        return ResultSuccess();
    }

    /* ... */

    void InstallTaskBase::IncrementProgress(s64 size) {
        std::scoped_lock lk(this->progress_mutex);
        this->progress.installed_size += size;
    }

    void InstallTaskBase::UpdateThroughputMeasurement(s64 throughput) {
        std::scoped_lock lk(this->throughput_mutex);

        /* Update throughput only if start time has been set. */
        if (this->throughput_start_time.GetNanoSeconds() != 0) {
            this->throughput.installed += throughput;
            this->throughput.elapsed_time = os::ConvertToTimeSpan(os::GetSystemTick()) - this->throughput_start_time;
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

    // Result InstallTaskBase::PrepareSystemUpdateDependency() {
    //     /* TODO */

    //     return ResultSuccess();
    // }

    Result InstallTaskBase::GetContentMetaInfoList(s32 *out_count, std::unique_ptr<ContentMetaInfo[]> *out_meta_infos, const ContentMetaKey &key) {
        /* Get the install content meta info. */
        InstallContentMetaInfo install_content_meta_info;
        R_TRY(this->GetInstallContentMetaInfo(std::addressof(install_content_meta_info), key));

        /* Open the BuiltInSystem content storage. */
        ContentStorage content_storage;
        R_TRY(ncm::OpenContentStorage(&content_storage, StorageId::BuiltInSystem));

        /* Write content meta to a placeholder. */
        InstallContentInfo content_info;
        R_TRY(this->WriteContentMetaToPlaceHolder(std::addressof(content_info), std::addressof(content_storage), install_content_meta_info, true));
        
        const PlaceHolderId placeholder_id = content_info.placeholder_id;
        
        /* Get the path of the new placeholder. */
        Path path;
        content_storage.GetPlaceHolderPath(std::addressof(path), placeholder_id);

        /* Read the variation list. */
        R_TRY(ReadVariationContentMetaInfoList(out_count, out_meta_infos, path, this->firmware_variation_id));

        /* Delete the placeholder. */
        content_storage.DeletePlaceHolder(placeholder_id);
        return ResultSuccess();
    }

    /* ... */

    Result InstallTaskBase::IsNewerThanInstalled(bool *out, const ContentMetaKey &key) {
        /* Obtain a list of suitable storage ids. */
        auto storage_list = GetStorageList(this->install_storage);

        /* Iterate over storage ids. */
        for (s32 i = 0; i < storage_list.Count(); i++) {
            /* Open the content meta database. */
            ContentMetaDatabase meta_db;
            R_TRY(OpenContentMetaDatabase(std::addressof(meta_db), storage_list[i]));
            
            /* Get the latest key. */
            ContentMetaKey installed_key;
            R_TRY_CATCH(meta_db.GetLatest(std::addressof(installed_key), key.id)) {
                R_CATCH(ncm::ResultContentMetaNotFound) { /* Key doesn't exist, this is okay. */ }
            } R_END_TRY_CATCH;

            /* Check if installed key is newer. */
            if (installed_key.version >= key.version) {
                *out = false;
                return ResultSuccess();
            }
        }

        /* Input key is newer. */
        *out = true;
        return ResultSuccess();
    }

    Result InstallTaskBase::DeleteInstallContentMetaData(const ContentMetaKey *keys, s32 num_keys) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(this->data->Count(std::addressof(count)));

        /* Delete the data if count < 1. */
        if (count < 1) {
            return this->data->Delete(keys, num_keys);
        }

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(this->data->Get(&content_meta, i));

            /* Cleanup if the input keys contain this key. */
            if (Contains(keys, num_keys, content_meta.GetReader().GetKey())) {
                R_TRY(this->CleanupOne(content_meta));
            }
        }

        return ResultSuccess();
    }

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

    /* ... */

    Result InstallTaskBase::FindMaxRequiredApplicationVersion(u32 *out) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(this->data->Count(std::addressof(count)));

        u32 max_version = 0;

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(this->data->Get(&content_meta, i));
            
            /* Create a reader. */
            const InstallContentMetaReader reader = content_meta.GetReader();

            /* Check if the meta type is for add on content. */
            if (reader.GetHeader()->type == ContentMetaType::AddOnContent) {
                const auto *extended_header = reader.GetExtendedHeader<AddOnContentMetaExtendedHeader>();
                
                /* Set the max version if higher. */
                if (extended_header->required_application_version >= max_version) {
                    max_version = extended_header->required_application_version;
                }
            }
        }

        *out = max_version;
        return ResultSuccess();
    }

    Result InstallTaskBase::FindMaxRequiredSystemVersion(u32 *out) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(this->data->Count(std::addressof(count)));

        u32 max_version = 0;

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(this->data->Get(&content_meta, i));
            
            /* Create a reader. */
            const InstallContentMetaReader reader = content_meta.GetReader();

            if (reader.GetHeader()->type == ContentMetaType::Application) {
                const auto *extended_header = reader.GetExtendedHeader<ApplicationMetaExtendedHeader>();

                /* Set the max version if higher. */
                if (extended_header->required_system_version >= max_version) {
                    max_version = extended_header->required_system_version;
                }
            } else if (reader.GetHeader()->type == ContentMetaType::Patch) {
                const auto *extended_header = reader.GetExtendedHeader<PatchMetaExtendedHeader>();

                /* Set the max version if higher. */
                if (extended_header->required_system_version >= max_version) {
                    max_version = extended_header->required_system_version;
                }
            }
        }

        *out = max_version;
        return ResultSuccess();
    }

    /* ... */

    Result InstallTaskBase::CanContinue() {
        auto progress = this->GetProgress();

        if (progress.state == InstallProgressState::NotPrepared || progress.state == InstallProgressState::DataPrepared) {
            R_UNLESS(!this->IsCancelRequested(), ncm::ResultCreatePlaceHolderCancelled());
        }

        if (progress.state == InstallProgressState::Prepared) {
            R_UNLESS(!this->IsCancelRequested(), ncm::ResultWritePlaceHolderCancelled());
        }

        return ResultSuccess();
    }

    void InstallTaskBase::SetFirmwareVariationId(FirmwareVariationId id) {
        this->firmware_variation_id = id;
    }
}
