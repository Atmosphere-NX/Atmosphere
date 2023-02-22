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
#include "ncm_extended_data_mapper.hpp"

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

        bool IsExpectedKey(const ContentMetaKey &expected_key, const ContentMetaKey &actual_key) {
            return expected_key.id      == actual_key.id      &&
                   expected_key.version == actual_key.version &&
                   expected_key.type    == actual_key.type;
        }

    }

    void InstallTaskBase::Cancel() {
        std::scoped_lock lk(m_cancel_mutex);
        m_cancel_requested = true;
    }

    void InstallTaskBase::ResetCancel() {
        std::scoped_lock lk(m_cancel_mutex);
        m_cancel_requested = false;
    }

    bool InstallTaskBase::IsCancelRequested() {
        std::scoped_lock lk(m_cancel_mutex);
        return m_cancel_requested;
    }

    Result InstallTaskBase::Initialize(StorageId install_storage, InstallTaskDataBase *data, u32 config) {
        R_UNLESS(IsInstallableStorage(install_storage), ncm::ResultUnknownStorage());

        m_install_storage = install_storage;
        m_data = data;
        m_config = config;

        R_RETURN(data->GetProgress(std::addressof(m_progress)));
    }

    Result InstallTaskBase::Prepare() {
        R_TRY(this->SetLastResultOnFailure(this->PrepareImpl()));
        R_SUCCEED();
    }

    Result InstallTaskBase::GetPreparedPlaceHolderPath(Path *out_path, u64 id, ContentMetaType meta_type, ContentType type) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(this->CountInstallContentMetaData(std::addressof(count)));

        /* Iterate over content meta. */
        util::optional<PlaceHolderId> placeholder_id;
        util::optional<StorageId> storage_id;
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(this->GetInstallContentMetaData(std::addressof(content_meta), i));
            const InstallContentMetaReader reader = content_meta.GetReader();

            /* Ensure content meta matches the key and meta type. */
            const auto key = reader.GetKey();

            if (key.id != id || key.type != meta_type) {
                continue;
            }

            /* Attempt to find a content info for the type. */
            for (size_t j = 0; j < reader.GetContentCount(); j++) {
                const auto content_info = reader.GetContentInfo(j);
                if (content_info->GetType() == type) {
                    placeholder_id = content_info->GetPlaceHolderId();
                    storage_id     = content_info->GetStorageId();
                    break;
                }
            }
        }

        R_UNLESS(placeholder_id, ncm::ResultPlaceHolderNotFound());
        R_UNLESS(storage_id,     ncm::ResultPlaceHolderNotFound());

        /* Open the relevant content storage. */
        ContentStorage storage;
        R_TRY(OpenContentStorage(std::addressof(storage), *storage_id));

        /* Get the path. */
        storage.GetPlaceHolderPath(out_path, *placeholder_id);

        R_SUCCEED();
    }

    Result InstallTaskBase::CalculateRequiredSize(s64 *out_size) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(m_data->Count(std::addressof(count)));

        s64 required_size = 0;
        /* Iterate over each entry. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(m_data->Get(std::addressof(content_meta), i));
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
        R_SUCCEED();
    }

    Result InstallTaskBase::PrepareImpl() {
        /* Reset the throughput. */
        this->ResetThroughputMeasurement();

        /* Transition from NotPrepared to DataPrepared. */
        if (this->GetProgress().state == InstallProgressState::NotPrepared) {
            R_TRY(this->PrepareInstallContentMetaData());
            R_TRY(this->PrepareDependency());
            R_TRY(this->CheckInstallable());
            this->SetProgressState(InstallProgressState::DataPrepared);
        }

        /* Transition from DataPrepared to Prepared. */
        if (this->GetProgress().state == InstallProgressState::DataPrepared) {
            R_TRY(this->PreparePlaceHolder());
            this->SetProgressState(InstallProgressState::Prepared);
        }

        /* Signal prepare is completed. */
        R_RETURN(this->OnPrepareComplete());
    }

    Result InstallTaskBase::Cleanup() {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(m_data->Count(std::addressof(count)));

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Get the content meta. */
            InstallContentMeta content_meta;
            R_TRY(m_data->Get(std::addressof(content_meta), i));

            /* Cleanup the content meta. */
            /* N doesn't check the result of this. */
            this->CleanupOne(content_meta);
        }

        /* Cleanup the data and progress. */
        R_TRY(m_data->Cleanup());
        this->CleanupProgress();

        R_SUCCEED();
    }

    Result InstallTaskBase::CleanupOne(const InstallContentMeta &content_meta) {
        /* Obtain a reader and get the storage id. */
        const auto reader = content_meta.GetReader();
        const auto storage_id = reader.GetStorageId();
        R_SUCCEED_IF(storage_id == StorageId::None);

        /* Open the relevant content storage. */
        ContentStorage content_storage;
        R_TRY(ncm::OpenContentStorage(std::addressof(content_storage), storage_id));

        /* Iterate over content infos. */
        for (size_t i = 0; i < reader.GetContentCount(); i++) {
            auto *content_info = reader.GetContentInfo(i);

            /* Delete placeholders for Prepared or Installed content infos. */
            if (content_info->install_state == InstallState::Prepared || content_info->install_state == InstallState::Installed) {
                content_storage.DeletePlaceHolder(content_info->placeholder_id);
            }
        }

        R_SUCCEED();
    }

    Result InstallTaskBase::ListContentMetaKey(s32 *out_keys_written, StorageContentMetaKey *out_keys, s32 out_keys_count, s32 offset, ListContentMetaKeyFilter filter) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(m_data->Count(std::addressof(count)));

        /* Offset exceeds keys that can be written. */
        if (count <= offset) {
            *out_keys_written = 0;
            R_SUCCEED();
        }

        if (filter == ListContentMetaKeyFilter::All) {
            const s32 num_keys = std::min(count, offset + out_keys_count);

            /* Iterate over content meta. */
            for (s32 i = offset; i < num_keys; i++) {
                /* Obtain the content meta. */
                InstallContentMeta content_meta;
                R_TRY(m_data->Get(std::addressof(content_meta), i));

                /* Write output StorageContentMetaKey. */
                const auto reader = content_meta.GetReader();
                out_keys[i - offset] = { reader.GetKey(), reader.GetStorageId() };
            }

            /* Output the number of keys written. */
            *out_keys_written = num_keys - offset;
        } else {
            s32 keys_written = 0;
            s32 cur_offset = 0;

            /* Iterate over content meta. */
            for (s32 i = 0; i < count; i++) {
                /* Obtain the content meta. */
                InstallContentMeta content_meta;
                R_TRY(m_data->Get(std::addressof(content_meta), i));

                /* Create a reader and check if the content has been committed. */
                const auto reader = content_meta.GetReader();
                const bool committed = reader.GetHeader()->committed;

                /* Apply filter. */
                if ((filter == ListContentMetaKeyFilter::Committed && committed) || (filter == ListContentMetaKeyFilter::NotCommitted && !committed)) {
                    /* Write output StorageContentMetaKey if at a suitable offset. */
                    if (cur_offset >= offset) {
                        out_keys[keys_written++] = { reader.GetKey(), reader.GetStorageId() };
                    }

                    /* Increment the current offset. */
                    cur_offset++;

                    /* We can't write any more output keys. */
                    if (keys_written >= out_keys_count) {
                        break;
                    }
                }
            }

            /* Output the number of keys written. */
            *out_keys_written = keys_written;
        }

        R_SUCCEED();
    }

    Result InstallTaskBase::ListApplicationContentMetaKey(s32 *out_keys_written, ApplicationContentMetaKey *out_keys, s32 out_keys_count, s32 offset) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(m_data->Count(std::addressof(count)));

        /* Offset exceeds keys that can be written. */
        if (count <= offset) {
            *out_keys_written = 0;
            R_SUCCEED();
        }

        /* Iterate over content meta. */
        const s32 max = std::min(count, offset + out_keys_count);
        s32 keys_written = 0;
        for (s32 i = offset; i < max; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(m_data->Get(std::addressof(content_meta), i));

            /* Create a reader. */
            const auto reader = content_meta.GetReader();

            /* Ensure this key has an application id. */
            const auto app_id = reader.GetApplicationId();
            if (!app_id) {
                continue;
            }

            /* Write output ApplicationContentMetaKey. */
            out_keys[keys_written++] = { reader.GetKey(), *app_id };
        }

        *out_keys_written = keys_written;
        R_SUCCEED();
    }

    Result InstallTaskBase::Execute() {
        R_TRY(this->SetLastResultOnFailure(this->ExecuteImpl()));
        R_SUCCEED();
    }

    Result InstallTaskBase::ExecuteImpl() {
        this->StartThroughputMeasurement();

        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(m_data->Count(std::addressof(count)));

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(m_data->Get(std::addressof(content_meta), i));

            /* Write all prepared content infos. */
            {
                /* If we fail while writing, update (but don't check the result). */
                ON_RESULT_FAILURE { m_data->Update(content_meta, i); };

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

            /* Update our data. */
            R_TRY(m_data->Update(content_meta, i));
        }

        /* Execution has finished, signal this and update the state. */
        R_TRY(this->OnExecuteComplete());

        this->SetProgressState(InstallProgressState::Downloaded);

        R_SUCCEED();
    }

    Result InstallTaskBase::PrepareAndExecute() {
        R_TRY(this->SetLastResultOnFailure(this->PrepareImpl()));
        R_TRY(this->SetLastResultOnFailure(this->ExecuteImpl()));
        R_SUCCEED();
    }

    Result InstallTaskBase::VerifyAllNotCommitted(const StorageContentMetaKey *keys, s32 num_keys) {
        /* No keys to check. */
        R_SUCCEED_IF(keys == nullptr);

        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(m_data->Count(std::addressof(count)));

        s32 num_not_committed = 0;

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(m_data->Get(std::addressof(content_meta), i));

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
        R_SUCCEED();
    }

    Result InstallTaskBase::CommitImpl(const StorageContentMetaKey *keys, s32 num_keys) {
        /* Ensure progress state is Downloaded. */
        R_UNLESS(this->GetProgress().state == InstallProgressState::Downloaded, ncm::ResultInvalidInstallTaskState());

        /* Ensure keys aren't committed. */
        R_TRY(this->VerifyAllNotCommitted(keys, num_keys));

        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(m_data->Count(std::addressof(count)));

        /* List of storages to commit. */
        StorageList commit_list;

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(m_data->Get(std::addressof(content_meta), i));

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

            /* Commit the current meta. */
            {
                /* Ensure that if something goes wrong during commit, we still try to update. */
                ON_RESULT_FAILURE { m_data->Update(content_meta, i); };

                /* Open a writer. */
                const auto writer = content_meta.GetWriter();

                /* Convert to content meta and store to a buffer. */
                std::unique_ptr<char[]> content_meta_buffer(new (std::nothrow) char[convert_size]);
                R_UNLESS(content_meta_buffer != nullptr, ncm::ResultAllocationFailed());
                reader.ConvertToContentMeta(content_meta_buffer.get(), convert_size);

                /* Open the content storage for this meta. */
                ContentStorage content_storage;
                R_TRY(OpenContentStorage(std::addressof(content_storage), storage_id));

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
            }

            /* Try to update our data. */
            R_TRY(m_data->Update(content_meta, i));
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

        R_SUCCEED();
    }

    Result InstallTaskBase::Commit(const StorageContentMetaKey *keys, s32 num_keys) {
        ON_RESULT_FAILURE { this->SetProgressState(InstallProgressState::Fatal); };

        R_RETURN(this->SetLastResultOnFailure(this->CommitImpl(keys, num_keys)));
    }

    Result InstallTaskBase::IncludesExFatDriver(bool *out) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(m_data->Count(std::addressof(count)));

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(m_data->Get(std::addressof(content_meta), i));

            /* Check if the attributes are set for including the exfat driver. */
            if (content_meta.GetReader().GetHeader()->attributes & ContentMetaAttribute_IncludesExFatDriver) {
                *out = true;
                R_SUCCEED();
            }
        }

        *out = false;
        R_SUCCEED();
    }

    Result InstallTaskBase::WritePlaceHolderBuffer(InstallContentInfo *content_info, const void *data, size_t data_size) {
        R_UNLESS(!this->IsCancelRequested(), ncm::ResultWritePlaceHolderCancelled());

        /* Open the content storage for the content info. */
        ContentStorage content_storage;
        R_TRY(OpenContentStorage(std::addressof(content_storage), content_info->storage_id));

        /* Write data to the placeholder. */
        R_TRY(content_storage.WritePlaceHolder(content_info->placeholder_id, content_info->written, data, data_size));
        content_info->written += data_size;

        /* Update progress/throughput if content info isn't temporary. */
        if (!content_info->is_temporary) {
            this->IncrementProgress(data_size);
            this->UpdateThroughputMeasurement(data_size);
        }

        /* Update the hash for the new data. */
        m_sha256_generator.Update(data, data_size);
        R_SUCCEED();
    }

    Result InstallTaskBase::WritePlaceHolder(const ContentMetaKey &key, InstallContentInfo *content_info) {
        if (content_info->is_sha256_calculated) {
            /* Update the hash with the buffered data. */
            m_sha256_generator.InitializeWithContext(std::addressof(content_info->context));
            m_sha256_generator.Update(content_info->buffered_data, content_info->buffered_data_size);
        } else {
            /* Initialize the generator. */
            m_sha256_generator.Initialize();
        }

        {
            ON_SCOPE_EXIT {
                /* Update this content info's sha256 data. */
                m_sha256_generator.GetContext(std::addressof(content_info->context));
                content_info->buffered_data_size = m_sha256_generator.GetBufferedDataSize();
                m_sha256_generator.GetBufferedData(content_info->buffered_data, m_sha256_generator.GetBufferedDataSize());
                content_info->is_sha256_calculated = true;
            };

            /* Perform the placeholder write. */
            R_TRY(this->OnWritePlaceHolder(key, content_info));
        }

        /* Compare generated hash to expected hash if verification required. */
        if (content_info->verify_digest) {
            u8 hash[crypto::Sha256Generator::HashSize];
            m_sha256_generator.GetHash(hash, crypto::Sha256Generator::HashSize);
            R_UNLESS(std::memcmp(hash, content_info->digest.data, crypto::Sha256Generator::HashSize) == 0, ncm::ResultInvalidContentHash());
        }

        if (hos::GetVersion() >= hos::Version_2_0_0 && !(m_config & InstallConfig_IgnoreTicket)) {
            ncm::RightsId rights_id;
            {
                /* Open the content storage and obtain the rights id. */
                ncm::ContentStorage storage;
                R_TRY(OpenContentStorage(std::addressof(storage), content_info->storage_id));
                R_TRY(storage.GetRightsId(std::addressof(rights_id), content_info->placeholder_id, content_info->GetContentAttributes()));
            }

            /* Install a ticket if necessary. */
            if (this->IsNecessaryInstallTicket(rights_id.id)) {
                R_TRY_CATCH(this->InstallTicket(rights_id.id, content_info->meta_type)) {
                    R_CATCH(ncm::ResultIgnorableInstallTicketFailure) { /* We can ignore the installation failure. */ }
                } R_END_TRY_CATCH;
            }
        }

        R_SUCCEED();
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

    Result InstallTaskBase::PreparePlaceHolder() {
        size_t total_size = 0;

        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(m_data->Count(std::addressof(count)));

        for (s32 i = 0; i < count; i++) {
            R_UNLESS(!this->IsCancelRequested(), ncm::ResultCreatePlaceHolderCancelled());

            AMS_FUNCTION_LOCAL_STATIC_CONSTINIT(os::SdkMutex, s_placeholder_mutex);
            std::scoped_lock lk(s_placeholder_mutex);

            InstallContentMeta content_meta;
            R_TRY(m_data->Get(std::addressof(content_meta), i));

            /* Update the data (and check result) when we are done. */
            {
                ON_RESULT_FAILURE { m_data->Update(content_meta, i); };

                /* Automatically choose a suitable storage id. */
                auto reader = content_meta.GetReader();
                StorageId storage_id = StorageId::None;
                if (reader.GetStorageId() != StorageId::None) {
                    storage_id = reader.GetStorageId();
                } else {
                    StorageId install_storage = this->GetInstallStorage();
                    R_TRY(ncm::SelectDownloadableStorage(std::addressof(storage_id), install_storage, reader.CalculateContentRequiredSize()));
                }

                /* Open the relevant content storage. */
                ContentStorage content_storage;
                R_TRY(ncm::OpenContentStorage(std::addressof(content_storage), storage_id));

                /* Update the storage id in the header. */
                auto writer = content_meta.GetWriter();
                writer.SetStorageId(storage_id);

                for (size_t j = 0; j < writer.GetContentCount(); j++) {
                    R_UNLESS(!this->IsCancelRequested(), ncm::ResultCreatePlaceHolderCancelled());
                    auto *content_info = writer.GetWritableContentInfo(j);

                    /* Check if we have the content already exists. */
                    bool has_content;
                    R_TRY(content_storage.Has(std::addressof(has_content), content_info->GetId()));

                    if (has_content) {
                        /* Add the size of installed content infos to the total size. */
                        if (content_info->install_state == InstallState::Installed) {
                            total_size += content_info->GetSize();
                        }

                        /* Update the install state. */
                        content_info->install_state = InstallState::AlreadyExists;

                        /* Continue. */
                        continue;
                    }

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

            /* Try to update our data. */
            R_TRY(m_data->Update(content_meta, i));
        }

        this->SetTotalSize(total_size);
        R_SUCCEED();
    }

    Result InstallTaskBase::WriteContentMetaToPlaceHolder(InstallContentInfo *out_install_content_info, ContentStorage *storage, const InstallContentMetaInfo &meta_info, util::optional<bool> is_temporary) {
        /* Generate a placeholder id. */
        auto placeholder_id = storage->GeneratePlaceHolderId();

        /* Create the placeholder. */
        R_TRY(storage->CreatePlaceHolder(placeholder_id, meta_info.content_id, meta_info.content_size));
        ON_RESULT_FAILURE { storage->DeletePlaceHolder(placeholder_id); };

        /* Output install content info. */
        *out_install_content_info = this->MakeInstallContentInfoFrom(meta_info, placeholder_id, is_temporary);

        /* Write install content info. */
        R_TRY(this->WritePlaceHolder(meta_info.key, out_install_content_info));

        /* Don't delete the placeholder. Set state to installed. */
        out_install_content_info->install_state = InstallState::Installed;
        R_SUCCEED();
    }

    Result InstallTaskBase::PrepareContentMeta(const InstallContentMetaInfo &meta_info, util::optional<ContentMetaKey> expected_key, util::optional<u32> source_version) {
        /* Open the BuiltInSystem content storage. */
        ContentStorage content_storage;
        R_TRY(OpenContentStorage(std::addressof(content_storage), StorageId::BuiltInSystem));

        /* Write content meta to a placeholder. */
        InstallContentInfo content_info;
        R_TRY(this->WriteContentMetaToPlaceHolder(std::addressof(content_info), std::addressof(content_storage), meta_info, util::nullopt));

        /* Get the path of the placeholder. */
        Path path;
        content_storage.GetPlaceHolderPath(std::addressof(path), content_info.GetPlaceHolderId());

        /* If we fail, delete the placeholder. */
        ON_RESULT_FAILURE { content_storage.DeletePlaceHolder(content_info.GetPlaceHolderId()); };

        /* Create a new temporary InstallContentInfo if relevant. */
        const bool is_temporary = content_info.is_temporary;
        if (is_temporary) {
            content_info = {
                .digest         = content_info.digest,
                .info           = content_info.info,
                .placeholder_id = content_info.GetPlaceHolderId(),
                .meta_type      = content_info.meta_type,
                .verify_digest  = content_info.verify_digest,
            };
        }

        /* Retrieve the install content meta data. */
        AutoBuffer meta;
        R_TRY(this->GetInstallContentMetaDataFromPath(std::addressof(meta), path, content_info, source_version));

        /* Update the storage id if BuiltInSystem. */
        if (m_install_storage == StorageId::BuiltInSystem) {
            InstallContentMetaWriter writer(meta.Get(), meta.GetSize());
            writer.SetStorageId(StorageId::BuiltInSystem);
        }

        /* Validate the expected key if we have an expectation. */
        if (expected_key) {
            InstallContentMetaReader reader(meta.Get(), meta.GetSize());
            R_UNLESS(IsExpectedKey(*expected_key, reader.GetKey()), ncm::ResultUnexpectedContentMetaPrepared());
        }

        /* Push the data. */
        R_TRY(m_data->Push(meta.Get(), meta.GetSize()));

        /* If the placeholder is temporary, delete it. */
        if (is_temporary) {
            content_storage.DeletePlaceHolder(content_info.GetPlaceHolderId());
        }

        R_SUCCEED();
    }

    Result InstallTaskBase::PrepareContentMeta(ContentId content_id, s64 size, ContentMetaType meta_type, AutoBuffer *buffer) {
        /* Create a reader. */
        PackagedContentMetaReader reader(buffer->Get(), buffer->GetSize());

        /* Initialize the temporary buffer. */
        AutoBuffer tmp_buffer;
        R_TRY(tmp_buffer.Initialize(reader.CalculateConvertInstallContentMetaSize()));

        /* Convert packaged content meta to install content meta. */
        reader.ConvertToInstallContentMeta(tmp_buffer.Get(), tmp_buffer.GetSize(), InstallContentInfo::Make(ContentInfo::Make(content_id, size, ContentInfo::DefaultContentAttributes, ContentType::Meta), meta_type));

        /* Push the content meta. */
        m_data->Push(tmp_buffer.Get(), tmp_buffer.GetSize());
        R_SUCCEED();
    }

    void InstallTaskBase::PrepareAgain() {
        this->SetProgressState(InstallProgressState::NotPrepared);
    }

    Result InstallTaskBase::PrepareDependency() {
        R_SUCCEED();
    }

    Result InstallTaskBase::PrepareSystemUpdateDependency() {
        /* Cleanup on failure. */
        ON_RESULT_FAILURE { this->Cleanup(); };

        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(this->CountInstallContentMetaData(std::addressof(count)));

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(m_data->Get(std::addressof(content_meta), i));

            /* Create a reader. */
            const InstallContentMetaReader reader = content_meta.GetReader();

            /* Skip non system update content metas. */
            if (reader.GetHeader()->type != ContentMetaType::SystemUpdate) {
                continue;
            }

            /* Get the content info. */
            const auto *content_info = reader.GetContentInfo(ContentType::Meta);

            /* List content meta infos. */
            std::unique_ptr<ContentMetaInfo[]> content_meta_infos;
            s32 num_content_meta_infos;
            R_TRY(this->ReadContentMetaInfoList(std::addressof(num_content_meta_infos), std::addressof(content_meta_infos), reader.GetKey(), content_info->GetContentAttributes()));

            /* Iterate over content meta infos. */
            for (s32 j = 0; j < num_content_meta_infos; j++) {
                ContentMetaInfo &content_meta_info = content_meta_infos[j];
                const ContentMetaKey content_meta_info_key = content_meta_info.ToKey();

                /* If exfat driver is not included or is required, prepare the content meta. */
                if (!(content_meta_info.attributes & ContentMetaAttribute_IncludesExFatDriver) || (m_config & InstallConfig_RequiresExFatDriver)) {
                    R_TRY(this->PrepareContentMetaIfLatest(content_meta_info_key));
                }
            }
        }

        R_SUCCEED();
    }

    Result InstallTaskBase::GetSystemUpdateTaskApplyInfo(SystemUpdateTaskApplyInfo *out) {
        /* Open the BuiltInSystem content meta database. */
        ContentMetaDatabase meta_db;
        R_TRY(OpenContentMetaDatabase(std::addressof(meta_db), StorageId::BuiltInSystem));

        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(m_data->Count(std::addressof(count)));

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(this->GetInstallContentMetaData(std::addressof(content_meta), i));

            /* Create a reader. */
            const InstallContentMetaReader reader = content_meta.GetReader();

            /* Skip non system update content metas. */
            if (reader.GetHeader()->type != ContentMetaType::SystemUpdate) {
                continue;
            }

            /* Get the content info. */
            const auto *content_info = reader.GetContentInfo(ContentType::Meta);

            /* List content meta infos. */
            std::unique_ptr<ContentMetaInfo[]> content_meta_infos;
            s32 num_content_meta_infos;
            R_TRY(this->ReadContentMetaInfoList(std::addressof(num_content_meta_infos), std::addressof(content_meta_infos), reader.GetKey(), content_info->GetContentAttributes()));

            /* Iterate over content meta infos. */
            for (s32 j = 0; j < num_content_meta_infos; j++) {
                const ContentMetaInfo &content_meta_info = content_meta_infos[j];
                bool found = true;

                /* Get the latest key. */
                ContentMetaKey installed_key;
                R_TRY_CATCH(meta_db.GetLatest(std::addressof(installed_key), content_meta_info.id)) {
                    R_CATCH(ncm::ResultContentMetaNotFound) {
                        /* Key doesn't exist, this is okay. */
                        found = false;
                    }
                } R_END_TRY_CATCH;

                /* Exfat driver included, but not required. */
                if (content_meta_info.attributes & ContentMetaAttribute_IncludesExFatDriver && !(m_config & InstallConfig_RequiresExFatDriver)) {
                    continue;
                }

                /* No reboot is required if we're installing a version below. */
                if (found && content_meta_info.version <= installed_key.version) {
                    continue;
                }

                /* If not rebootless, a reboot is required. */
                if (!(content_meta_info.attributes & ContentMetaAttribute_Rebootless)) {
                    *out = SystemUpdateTaskApplyInfo::RequireReboot;
                    R_SUCCEED();
                }
            }
        }

        *out = SystemUpdateTaskApplyInfo::RequireNoReboot;
        R_SUCCEED();
    }

    Result InstallTaskBase::PrepareContentMetaIfLatest(const ContentMetaKey &key) {
        /* Check if the key is newer than what is already installed. */
        bool newer_than_installed;
        R_TRY(this->IsNewerThanInstalled(std::addressof(newer_than_installed), key));

        if (newer_than_installed) {
            /* Get and prepare install content meta info. */
            InstallContentMetaInfo install_content_meta_info;
            R_TRY(this->GetInstallContentMetaInfo(std::addressof(install_content_meta_info), key));
            R_TRY(this->PrepareContentMeta(install_content_meta_info, key, util::nullopt));
        }

        R_SUCCEED();
    }

    Result InstallTaskBase::IsNewerThanInstalled(bool *out, const ContentMetaKey &key) {
        /* Obtain a list of suitable storage ids. */
        auto storage_list = GetStorageList(m_install_storage);

        /* Iterate over storage ids. */
        for (s32 i = 0; i < storage_list.Count(); i++) {
            /* Open the content meta database. */
            ContentMetaDatabase meta_db;
            if (R_FAILED(OpenContentMetaDatabase(std::addressof(meta_db), storage_list[i]))) {
                continue;
            }

            /* Get the latest key. */
            ContentMetaKey latest_key;
            R_TRY_CATCH(meta_db.GetLatest(std::addressof(latest_key), key.id)) {
                R_CATCH(ncm::ResultContentMetaNotFound) {
                    /* Key doesn't exist, this is okay. */
                    continue;
                }
            } R_END_TRY_CATCH;

            /* Check if installed key is newer. */
            if (latest_key.version >= key.version) {
                *out = false;
                R_SUCCEED();
            }
        }

        /* Input key is newer. */
        *out = true;
        R_SUCCEED();
    }

    Result InstallTaskBase::CountInstallContentMetaData(s32 *out_count) {
        R_RETURN(m_data->Count(out_count));
    }

    Result InstallTaskBase::GetInstallContentMetaData(InstallContentMeta *out_content_meta, s32 index) {
        R_RETURN(m_data->Get(out_content_meta, index));
    }

    Result InstallTaskBase::DeleteInstallContentMetaData(const ContentMetaKey *keys, s32 num_keys) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(this->CountInstallContentMetaData(std::addressof(count)));

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(this->GetInstallContentMetaData(std::addressof(content_meta), i));

            /* Cleanup if the input keys contain this key. */
            if (Contains(keys, num_keys, content_meta.GetReader().GetKey())) {
                R_TRY(this->CleanupOne(content_meta));
            }
        }

        /* Delete the data if count < 1. */
        R_RETURN(m_data->Delete(keys, num_keys));
    }

    Result InstallTaskBase::GetInstallContentMetaDataFromPath(AutoBuffer *out, const Path &path, const InstallContentInfo &content_info, util::optional<u32> source_version) {
        fs::ContentAttributes attr;
        AutoBuffer meta;
        R_TRY(TryReadContentMetaPath(std::addressof(attr), std::addressof(meta), path.str, ReadContentMetaPathWithoutExtendedDataOrDigestSuppressingFsAbort));

        /* Create a reader. */
        PackagedContentMetaReader reader(meta.Get(), meta.GetSize());
        size_t meta_size;

        AutoBuffer install_meta_data;
        if (source_version) {
            /* Declare buffers. */
            constexpr size_t BufferCount = 2;
            constexpr size_t BufferSize  = 3_KB;
            u8 buffers[BufferCount][BufferSize];

            /* Create a mapper. */
            auto mapper = MultiCacheReadonlyMapper<4>(Span<u8>(buffers[0], sizeof(buffers[0])), Span<u8>(buffers[1], sizeof(buffers[1])));
            R_TRY(mapper.Initialize(path.str, static_cast<fs::ContentAttributes>(static_cast<u8>(attr) & fs::ContentAttributes_All), true));

            /* Create an accessor. */
            auto accessor = PatchMetaExtendedDataAccessor{std::addressof(mapper)};

            /* Convert to fragment only install meta. */
            R_TRY(MetaConverter::GetFragmentOnlyInstallContentMeta(std::addressof(install_meta_data), content_info, reader, std::addressof(accessor), source_version.value()));
        } else {
            /* Convert to install content meta. */
            meta_size = reader.CalculateConvertInstallContentMetaSize();
            R_TRY(install_meta_data.Initialize(meta_size));
            reader.ConvertToInstallContentMeta(install_meta_data.Get(), install_meta_data.GetSize(), content_info);
        }

        /* Set output. */
        *out = std::move(install_meta_data);

        R_SUCCEED();
    }

    InstallContentInfo InstallTaskBase::MakeInstallContentInfoFrom(const InstallContentMetaInfo &info, const PlaceHolderId &placeholder_id, util::optional<bool> is_tmp) {
        return {
            .digest         = info.digest,
            .info           = ContentInfo::Make(info.content_id, info.content_size, ContentInfo::DefaultContentAttributes, ContentType::Meta, 0),
            .placeholder_id = placeholder_id,
            .meta_type      = info.key.type,
            .install_state  = InstallState::Prepared,
            .verify_digest  = info.verify_digest,
            .storage_id     = StorageId::BuiltInSystem,
            .is_temporary   = is_tmp ? *is_tmp : (m_install_storage != StorageId::BuiltInSystem),
        };
    }

    InstallProgress InstallTaskBase::GetProgress() {
        std::scoped_lock lk(m_progress_mutex);
        return m_progress;
    }

    void InstallTaskBase::ResetLastResult() {
        this->SetLastResult(ResultSuccess());
    }

    void InstallTaskBase::SetTotalSize(s64 size) {
        std::scoped_lock lk(m_progress_mutex);
        m_progress.total_size = size;
    }

    void InstallTaskBase::IncrementProgress(s64 size) {
        std::scoped_lock lk(m_progress_mutex);
        m_progress.installed_size += size;
    }

    void InstallTaskBase::SetLastResult(Result last_result) {
        std::scoped_lock lk(m_progress_mutex);
        m_data->SetLastResult(last_result);
        m_progress.SetLastResult(last_result);
    }

    void InstallTaskBase::CleanupProgress() {
        std::scoped_lock lk(m_progress_mutex);
        m_progress = {};
    }

    InstallThroughput InstallTaskBase::GetThroughput() {
        std::scoped_lock lk(m_throughput_mutex);
        return m_throughput;
    }

    void InstallTaskBase::ResetThroughputMeasurement() {
        std::scoped_lock lk(m_throughput_mutex);
        m_throughput = { .elapsed_time = TimeSpan() };
        m_throughput_start_time = TimeSpan();
    }

    void InstallTaskBase::StartThroughputMeasurement() {
        std::scoped_lock lk(m_throughput_mutex);
        m_throughput = { .elapsed_time = TimeSpan() };
        m_throughput_start_time = os::GetSystemTick().ToTimeSpan();
    }

    void InstallTaskBase::UpdateThroughputMeasurement(s64 throughput) {
        std::scoped_lock lk(m_throughput_mutex);

        /* Update throughput only if start time has been set. */
        if (m_throughput_start_time.GetNanoSeconds() != 0) {
            m_throughput.installed += throughput;
            m_throughput.elapsed_time = os::GetSystemTick().ToTimeSpan() - m_throughput_start_time;
        }
    }

    Result InstallTaskBase::CalculateContentsSize(s64 *out_size, const ContentMetaKey &key, StorageId storage_id) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(m_data->Count(std::addressof(count)));

        /* Open the content storage. */
        ContentStorage content_storage;
        R_TRY(ncm::OpenContentStorage(std::addressof(content_storage), storage_id));

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(m_data->Get(std::addressof(content_meta), i));

            /* Create a reader. */
            const InstallContentMetaReader reader = content_meta.GetReader();

            /* Skip this content meta if the key doesn't match. */
            if (reader.GetKey() != key) {
                continue;
            }

            /* If the storage is unique and doesn't match, continue. */
            if (IsUniqueStorage(storage_id) && reader.GetStorageId() != storage_id) {
                continue;
            }

            /* Set the out size to 0. */
            *out_size = 0;

            /* Sum the sizes from the content infos. */
            for (size_t j = 0; j < reader.GetContentCount(); j++) {
                const auto *content_info = reader.GetContentInfo(j);

                /* If this content info isn't prepared, continue. */
                if (content_info->install_state == InstallState::NotPrepared) {
                    continue;
                }

                /* Check if this content info has a placeholder. */
                bool has_placeholder;
                R_TRY(content_storage.HasPlaceHolder(std::addressof(has_placeholder), content_info->GetPlaceHolderId()));

                /* Add the placeholder size to the total. */
                if (has_placeholder) {
                    *out_size += content_info->GetSize();
                }
            }

            /* No need to look for any further keys. */
            R_SUCCEED();
        }

        *out_size = 0;
        R_SUCCEED();
    }

    Result InstallTaskBase::ReadContentMetaInfoList(s32 *out_count, std::unique_ptr<ContentMetaInfo[]> *out_meta_infos, const ContentMetaKey &key, fs::ContentAttributes attr) {
        /* Get the install content meta info. */
        InstallContentMetaInfo install_content_meta_info;
        R_TRY(this->GetInstallContentMetaInfo(std::addressof(install_content_meta_info), key));

        /* Open the BuiltInSystem content storage. */
        ContentStorage content_storage;
        R_TRY(ncm::OpenContentStorage(std::addressof(content_storage), StorageId::BuiltInSystem));

        /* Write content meta to a placeholder. */
        InstallContentInfo content_info;
        R_TRY(this->WriteContentMetaToPlaceHolder(std::addressof(content_info), std::addressof(content_storage), install_content_meta_info, true));

        const PlaceHolderId placeholder_id = content_info.GetPlaceHolderId();

        /* Get the path of the new placeholder. */
        Path path;
        content_storage.GetPlaceHolderPath(std::addressof(path), placeholder_id);

        /* Read the variation list. */
        R_TRY(ReadVariationContentMetaInfoList(out_count, out_meta_infos, path, attr, m_firmware_variation_id));

        /* Delete the placeholder. */
        content_storage.DeletePlaceHolder(placeholder_id);
        R_SUCCEED();
    }

    Result InstallTaskBase::FindMaxRequiredApplicationVersion(u32 *out) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(m_data->Count(std::addressof(count)));

        u32 max_version = 0;

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(m_data->Get(std::addressof(content_meta), i));

            /* Create a reader. */
            const InstallContentMetaReader reader = content_meta.GetReader();

            /* Check if the meta type is for add on content. */
            if (reader.GetKey().type == ContentMetaType::AddOnContent) {
                const auto *extended_header = reader.GetExtendedHeader<AddOnContentMetaExtendedHeader>();

                /* Set the max version if higher. */
                max_version = std::max(max_version, extended_header->required_application_version);
            }
        }

        *out = max_version;
        R_SUCCEED();
    }

    Result InstallTaskBase::ListOccupiedSize(s32 *out_written, InstallTaskOccupiedSize *out_list, s32 out_list_size, s32 offset) {
        AMS_ABORT_UNLESS(offset >= 0);

        /* Count the number of content meta entries. */
        s32 data_count;
        R_TRY(m_data->Count(std::addressof(data_count)));

        /* Iterate over content meta. */
        s32 count = 0;
        for (s32 i = offset; i < data_count && count < out_list_size; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(m_data->Get(std::addressof(content_meta), i));

            /* Create a reader. */
            const InstallContentMetaReader reader = content_meta.GetReader();
            const StorageId storage_id = reader.GetStorageId();

            s64 total_size = 0;

            for (size_t j = 0; j < reader.GetContentCount(); j++) {
                const auto *content_info = reader.GetContentInfo(j);

                /* Skip the content info if not prepared. */
                if (content_info->GetInstallState() == InstallState::NotPrepared) {
                    continue;
                }

                /* Open the relevant content storage. */
                ContentStorage content_storage;
                R_TRY_CATCH(ncm::OpenContentStorage(std::addressof(content_storage), storage_id)) {
                    R_CATCH(ncm::ResultContentStorageNotActive) { break; }
                } R_END_TRY_CATCH;

                /* Check if this content info has a placeholder. */
                bool has_placeholder;
                R_TRY(content_storage.HasPlaceHolder(std::addressof(has_placeholder), content_info->GetPlaceHolderId()));

                if (has_placeholder) {
                    total_size += content_info->GetSize();
                }
            }

            /* Output this InstallTaskOccupiedSize. */
            out_list[count++] = {
                .key = reader.GetKey(),
                .size = total_size,
                .storage_id = storage_id,
            };
        }

        /* Write the out count. */
        *out_written = count;

        R_SUCCEED();
    }

    void InstallTaskBase::SetProgressState(InstallProgressState state) {
        std::scoped_lock lk(m_progress_mutex);
        m_data->SetState(state);
        m_progress.state = state;
    }

    Result InstallTaskBase::FindMaxRequiredSystemVersion(u32 *out) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(m_data->Count(std::addressof(count)));

        u32 max_version = 0;

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(m_data->Get(std::addressof(content_meta), i));

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
        R_SUCCEED();
    }

    Result InstallTaskBase::CanContinue() {
        switch (this->GetProgress().state) {
            case InstallProgressState::NotPrepared:
            case InstallProgressState::DataPrepared:
                R_UNLESS(!this->IsCancelRequested(), ncm::ResultCreatePlaceHolderCancelled());
                break;
            case InstallProgressState::Prepared:
                R_UNLESS(!this->IsCancelRequested(), ncm::ResultWritePlaceHolderCancelled());
                break;
            default:
                break;
        }

        R_SUCCEED();
    }

    Result InstallTaskBase::ListRightsIds(s32 *out_count, Span<RightsId> out_span, const ContentMetaKey &key, s32 offset) {
        /* Count the number of content meta entries. */
        s32 count;
        R_TRY(m_data->Count(std::addressof(count)));

        /* Ensure count is >= 1. */
        R_UNLESS(count >= 1, ncm::ResultContentMetaNotFound());

        /* Iterate over content meta. */
        for (s32 i = 0; i < count; i++) {
            /* Obtain the content meta. */
            InstallContentMeta content_meta;
            R_TRY(m_data->Get(std::addressof(content_meta), i));

            /* Create a reader. */
            const InstallContentMetaReader reader = content_meta.GetReader();

            /* List rights ids if the reader's key matches ours. */
            if (reader.GetKey() == key) {
                R_RETURN(this->ListRightsIdsByInstallContentMeta(out_count, out_span, content_meta, offset));
            }
        }

        R_THROW(ncm::ResultContentMetaNotFound());
    }

    Result InstallTaskBase::ListRightsIdsByInstallContentMeta(s32 *out_count, Span<RightsId> out_span, const InstallContentMeta &content_meta, s32 offset) {
        /* If the offset is greater than zero, we can't create a unique span of rights ids. */
        /* Thus, we have nothing to list. */
        if (offset > 0) {
            *out_count = 0;
            R_SUCCEED();
        }

        /* Create a reader. */
        const InstallContentMetaReader reader = content_meta.GetReader();

        s32 count = 0;
        for (size_t i = 0; i < reader.GetContentCount(); i++) {
            const auto *content_info = reader.GetContentInfo(i);

            /* Skip meta content infos and already installed content infos. Also skip if the content meta has already been comitted. */
            if (content_info->GetType() == ContentType::Meta || content_info->GetInstallState() == InstallState::Installed || reader.GetHeader()->committed) {
                continue;
            }

            /* Open the relevant content storage. */
            ContentStorage content_storage;
            R_TRY(ncm::OpenContentStorage(std::addressof(content_storage), content_info->storage_id));

            /* Get the rights id. */
            RightsId rights_id;
            R_TRY(content_storage.GetRightsId(std::addressof(rights_id), content_info->GetPlaceHolderId(), content_info->GetContentAttributes()));

            /* Skip empty rights ids. */
            if (rights_id.id == fs::InvalidRightsId) {
                continue;
            }

            /* Output the rights id. */
            out_span[count++] = rights_id;
        }

        /* Sort and remove duplicate ids from the output span. */
        std::sort(out_span.begin(), out_span.end());
        *out_count = std::distance(out_span.begin(), std::unique(out_span.begin(), out_span.end()));
        R_SUCCEED();
    }
}
