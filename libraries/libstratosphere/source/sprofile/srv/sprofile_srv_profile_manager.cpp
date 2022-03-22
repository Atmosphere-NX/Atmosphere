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
#include "sprofile_srv_profile_manager.hpp"
#include "sprofile_srv_fs_utils.hpp"

namespace ams::sprofile::srv {

    namespace {

        constexpr const char PrimaryDirectoryName[]   = "primary";
        constexpr const char TemporaryDirectoryName[] = "temp";

        Result CreateSaveData(const ProfileManager::SaveDataInfo &save_data_info) {
            R_TRY_CATCH(fs::CreateSystemSaveData(save_data_info.id, save_data_info.size, save_data_info.journal_size, save_data_info.flags)) {
                R_CATCH(fs::ResultPathAlreadyExists) { /* Nintendo accepts already-existing savedata here. */ }
            } R_END_TRY_CATCH;
            return ResultSuccess();
        }

        void SafePrint(char *dst, size_t dst_size, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

        void SafePrint(char *dst, size_t dst_size, const char *fmt, ...) {
            std::va_list vl;
            va_start(vl, fmt);
            const size_t len = util::TVSNPrintf(dst, dst_size, fmt, vl);
            va_end(vl);

            AMS_ABORT_UNLESS(len < dst_size);
        }

        void CreateMetadataPathImpl(char *dst, size_t dst_size, const char *mount, const char *dir) {
            SafePrint(dst, dst_size, "%s:/%s/metadata", mount, dir);
        }

        void CreatePrimaryMetadataPath(char *dst, size_t dst_size, const char *mount) {
            CreateMetadataPathImpl(dst, dst_size, mount, PrimaryDirectoryName);
        }

        void CreateTemporaryMetadataPath(char *dst, size_t dst_size, const char *mount) {
            CreateMetadataPathImpl(dst, dst_size, mount, TemporaryDirectoryName);
        }

        void CreateProfilePathImpl(char *dst, size_t dst_size, const char *mount, const char *dir, const Identifier &id) {
            SafePrint(dst, dst_size, "%s:/%s/profiles/%02x%02x%02x%02x%02x%02x%02x", mount, dir, id.data[0], id.data[1], id.data[2], id.data[3], id.data[4], id.data[5], id.data[6]);
        }

        void CreatePrimaryProfilePath(char *dst, size_t dst_size, const char *mount, const Identifier &id) {
            CreateProfilePathImpl(dst, dst_size, mount, PrimaryDirectoryName, id);
        }

        void CreateTemporaryProfilePath(char *dst, size_t dst_size, const char *mount, const Identifier &id) {
            CreateProfilePathImpl(dst, dst_size, mount, TemporaryDirectoryName, id);
        }

        void CreateDirectoryPathImpl(char *dst, size_t dst_size, const char *mount, const char *dir)  {
            SafePrint(dst, dst_size, "%s:/%s", mount, dir);
        }

        void CreatePrimaryDirectoryPath(char *dst, size_t dst_size, const char *mount) {
            CreateDirectoryPathImpl(dst, dst_size, mount, PrimaryDirectoryName);
        }

        void CreateTemporaryDirectoryPath(char *dst, size_t dst_size, const char *mount) {
            CreateDirectoryPathImpl(dst, dst_size, mount, TemporaryDirectoryName);
        }

        void CreateProfileDirectoryPathImpl(char *dst, size_t dst_size, const char *mount, const char *dir)  {
            SafePrint(dst, dst_size, "%s:/%s/profiles", mount, dir);
        }

        void CreatePrimaryProfileDirectoryPath(char *dst, size_t dst_size, const char *mount) {
            CreateProfileDirectoryPathImpl(dst, dst_size, mount, PrimaryDirectoryName);
        }

        void CreateTemporaryProfileDirectoryPath(char *dst, size_t dst_size, const char *mount) {
            CreateProfileDirectoryPathImpl(dst, dst_size, mount, TemporaryDirectoryName);
        }

    }

    ProfileManager::ProfileManager(const SaveDataInfo &save_data_info)
        : m_save_data_info(save_data_info), m_save_file_mounted(false), m_profile_importer(util::nullopt),
          m_profile_metadata(util::nullopt), m_service_profile(util::nullopt), m_update_observer_manager()
    {
        /* ... */
    }

    void ProfileManager::InitializeSaveData() {
        /* Acquire locks. */
        std::scoped_lock lk1(m_general_mutex);
        std::scoped_lock lk2(m_fs_mutex);

        /* Ensure the savedata exists. */
        if (R_SUCCEEDED(CreateSaveData(m_save_data_info))) {
            m_save_file_mounted = R_SUCCEEDED(fs::MountSystemSaveData(m_save_data_info.mount_name, m_save_data_info.id));
        }
    }

    Result ProfileManager::ResetSaveData() {
        /* Acquire locks. */
        std::scoped_lock lk1(m_service_profile_mutex);
        std::scoped_lock lk2(m_profile_metadata_mutex);
        std::scoped_lock lk3(m_general_mutex);
        std::scoped_lock lk4(m_fs_mutex);

        /* Unmount save file. */
        fs::Unmount(m_save_data_info.mount_name);
        m_save_file_mounted = false;

        /* Delete save file. */
        R_TRY(fs::DeleteSystemSaveData(fs::SaveDataSpaceId::System, m_save_data_info.id, fs::InvalidUserId));

        /* Unload profile. */
        m_profile_metadata = util::nullopt;
        m_service_profile  = util::nullopt;

        /* Create the save data. */
        R_TRY(CreateSaveData(m_save_data_info));

        /* Try to mount the save file. */
        const auto result = fs::MountSystemSaveData(m_save_data_info.mount_name, m_save_data_info.id);
        m_save_file_mounted = R_SUCCEEDED(result);

        return result;
    }

    Result ProfileManager::OpenProfileImporter() {
        /* Acquire locks. */
        std::scoped_lock lk1(m_profile_metadata_mutex);
        std::scoped_lock lk2(m_profile_importer_mutex);

        /* Check that we don't already have an importer. */
        R_UNLESS(!m_profile_importer.has_value(), sprofile::ResultInvalidState());

        /* Create importer. */
        m_profile_importer.emplace(m_profile_metadata);
        return ResultSuccess();
    }

    void ProfileManager::CloseProfileImporter() {
        /* Acquire locks. */
        std::scoped_lock lk1(m_profile_importer_mutex);
        std::scoped_lock lk2(m_general_mutex);
        std::scoped_lock lk3(m_fs_mutex);

        /* Close our importer. */
        m_profile_importer = util::nullopt;
    }

    Result ProfileManager::ImportProfile(const sprofile::srv::ProfileDataForImportData &import) {
        /* Acquire locks. */
        std::scoped_lock lk1(m_profile_importer_mutex);
        std::scoped_lock lk2(m_fs_mutex);

        /* Check that we have an importer. */
        R_UNLESS(m_profile_importer.has_value(), sprofile::ResultInvalidState());

        /* Check that the metadata we're importing is a valid version. */
        R_UNLESS(IsValidProfileFormatVersion(import.header.version), sprofile::ResultInvalidDataVersion());

        /* Check that the metadata we're importing has a valid hash. */
        {
            crypto::Md5Generator md5;
            md5.Initialize();

            md5.Update(std::addressof(import.header), sizeof(import.header));
            md5.Update(std::addressof(import.data), sizeof(import.data) - sizeof(import.data.entries[0]) * (util::size(import.data.entries) - std::min<size_t>(import.data.num_entries, util::size(import.data.entries))));

            u8 hash[crypto::Md5Generator::HashSize];
            md5.GetHash(hash, sizeof(hash));

            R_UNLESS(crypto::IsSameBytes(hash, import.hash, sizeof(hash)), sprofile::ResultInvalidDataHash());
        }

        /* Succeed if we already have the profile. */
        R_SUCCEED_IF(m_profile_importer->HasProfile(import.header.identifier_0, import.header.identifier_1));

        /* Check that we're importing the profile. */
        R_UNLESS(m_profile_importer->CanImportProfile(import.header.identifier_0), sprofile::ResultInvalidState());

        /* Create temporary directories. */
        R_TRY(this->EnsureTemporaryDirectories());

        /* Create profile. */
        char path[0x30];
        CreateTemporaryProfilePath(path, sizeof(path), m_save_data_info.mount_name, import.header.identifier_0);
        R_TRY(WriteFile(path, std::addressof(import.data), sizeof(import.data)));

        /* Set profile imported. */
        m_profile_importer->OnImportProfile(import.header.identifier_0);
        return ResultSuccess();
    }

    Result ProfileManager::Commit() {
        /* Acquire locks. */
        std::scoped_lock lk1(m_service_profile_mutex);
        std::scoped_lock lk2(m_profile_metadata_mutex);
        std::scoped_lock lk3(m_profile_importer_mutex);
        std::scoped_lock lk4(m_general_mutex);
        std::scoped_lock lk5(m_fs_mutex);

        /* Check that we have an importer. */
        R_UNLESS(m_profile_importer.has_value(), sprofile::ResultInvalidState());

        /* Commit, and if we fail remount our save. */
        {
            /* Setup guard in case we fail. */
            auto remount_guard = SCOPE_GUARD {
                if (m_profile_importer.has_value()) {
                    /* Unmount save file. */
                    fs::Unmount(m_save_data_info.mount_name);
                    m_save_file_mounted = false;

                    /* Re-mount save file. */
                    R_ABORT_UNLESS(fs::MountSystemSaveData(m_save_data_info.mount_name, m_save_data_info.id));
                    m_save_file_mounted = true;

                    /* Reset our importer. */
                    m_profile_importer = util::nullopt;
                }
            };

            /* Check that we can commit the importer. */
            R_UNLESS(m_profile_importer->CanCommit(), sprofile::ResultInvalidState());

            /* Commit. */
            R_TRY(this->CommitImpl());

            /* Commit the save file. */
            R_TRY(fs::CommitSaveData(m_save_data_info.mount_name));

            /* We successfully committed. */
            remount_guard.Cancel();
        }

        /* NOTE: Here nintendo generates an "sprofile_update_profile" sreport with the new and old revision keys. */

        /* Handle tasks for when we've committed (including notifying update observers). */
        this->OnCommitted();

        return ResultSuccess();
    }

    Result ProfileManager::ImportMetadata(const sprofile::srv::ProfileMetadataForImportMetadata &import) {
        /* Acquire locks. */
        std::scoped_lock lk1(m_profile_importer_mutex);
        std::scoped_lock lk2(m_fs_mutex);

        /* Check that we can import metadata. */
        R_UNLESS(m_profile_importer.has_value(),          sprofile::ResultInvalidState());
        R_UNLESS(m_profile_importer->CanImportMetadata(), sprofile::ResultInvalidState());

        /* Check that the metadata we're importing is a valid version. */
        R_UNLESS(IsValidProfileFormatVersion(import.header.version), sprofile::ResultInvalidMetadataVersion());

        /* Check that the metadata we're importing has a valid hash. */
        {
            crypto::Md5Generator md5;
            md5.Initialize();

            md5.Update(std::addressof(import.header), sizeof(import.header));
            md5.Update(std::addressof(import.metadata), sizeof(import.metadata));
            md5.Update(std::addressof(import.profile_urls), sizeof(import.profile_urls[0]) * std::min<size_t>(import.metadata.num_entries, util::size(import.metadata.entries)));

            u8 hash[crypto::Md5Generator::HashSize];
            md5.GetHash(hash, sizeof(hash));

            R_UNLESS(crypto::IsSameBytes(hash, import.hash, sizeof(hash)), sprofile::ResultInvalidMetadataHash());
        }

        /* Create temporary directories. */
        R_TRY(this->EnsureTemporaryDirectories());

        /* Create metadata. */
        char path[0x30];
        CreateTemporaryMetadataPath(path, sizeof(path), m_save_data_info.mount_name);
        R_TRY(WriteFile(path, std::addressof(import.metadata), sizeof(import.metadata)));

        /* Import the metadata. */
        m_profile_importer->ImportMetadata(import.metadata);
        return ResultSuccess();
    }

    Result ProfileManager::LoadPrimaryMetadata(ProfileMetadata *out) {
        /* Acquire locks. */
        std::scoped_lock lk1(m_profile_metadata_mutex);
        std::scoped_lock lk2(m_general_mutex);

        /* If we don't have metadata, load it. */
        if (!m_profile_metadata.has_value()) {
            /* Emplace our metadata. */
            m_profile_metadata.emplace();
            auto meta_guard = SCOPE_GUARD { m_profile_metadata = util::nullopt; };

            /* Read profile metadata. */
            char path[0x30];
            CreatePrimaryMetadataPath(path, sizeof(path), m_save_data_info.mount_name);
            R_TRY(ReadFile(path, std::addressof(*m_profile_metadata), sizeof(*m_profile_metadata), 0));

            /* We read the metadata successfully. */
            meta_guard.Cancel();
        }

        /* Set the output. */
        *out = *m_profile_metadata;
        return ResultSuccess();
    }

    Result ProfileManager::LoadProfile(Identifier profile) {
        /* Check if we already have the profile. */
        if (m_service_profile.has_value()) {
            R_SUCCEED_IF(m_service_profile->name == profile);
        }

        /* If we fail past this point, we want to have no profile. */
        auto prof_guard = SCOPE_GUARD { m_service_profile = util::nullopt; };

        /* Create profile path. */
        char path[0x30];
        CreatePrimaryProfilePath(path, sizeof(path), m_save_data_info.mount_name, profile);

        /* Load the profile. */
        m_service_profile = {};
        R_TRY(ReadFile(path, std::addressof(m_service_profile->data), sizeof(m_service_profile->data), 0));

        /* We succeeded. */
        prof_guard.Cancel();
        return ResultSuccess();
    }

    Result ProfileManager::GetDataEntry(ProfileDataEntry *out, Identifier profile, Identifier key) {
        /* Acquire locks. */
        std::scoped_lock lk1(m_service_profile_mutex);
        std::scoped_lock lk2(m_general_mutex);

        /* Load the desired profile. */
        if (R_SUCCEEDED(this->LoadProfile(profile))) {
            /* Find the specified key. */
            for (auto i = 0u; i < std::min<size_t>(m_service_profile->data.num_entries, util::size(m_service_profile->data.entries)); ++i) {
                if (m_service_profile->data.entries[i].key == key) {
                    *out = m_service_profile->data.entries[i];
                    return ResultSuccess();
                }
            }
        }

        return sprofile::ResultKeyNotFound();
    }

    Result ProfileManager::GetSigned64(s64 *out, Identifier profile, Identifier key) {
        /* Get the data entry. */
        ProfileDataEntry entry;
        R_TRY(this->GetDataEntry(std::addressof(entry), profile, key));

        /* Check the type. */
        R_UNLESS(entry.type == ValueType_S64, sprofile::ResultInvalidDataType());

        /* Set the output value. */
        *out = entry.value_s64;
        return ResultSuccess();
    }

    Result ProfileManager::GetUnsigned64(u64 *out, Identifier profile, Identifier key) {
        /* Get the data entry. */
        ProfileDataEntry entry;
        R_TRY(this->GetDataEntry(std::addressof(entry), profile, key));

        /* Check the type. */
        R_UNLESS(entry.type == ValueType_U64, sprofile::ResultInvalidDataType());

        /* Set the output value. */
        *out = entry.value_u64;
        return ResultSuccess();
    }

    Result ProfileManager::GetSigned32(s32 *out, Identifier profile, Identifier key) {
        /* Get the data entry. */
        ProfileDataEntry entry;
        R_TRY(this->GetDataEntry(std::addressof(entry), profile, key));

        /* Check the type. */
        R_UNLESS(entry.type == ValueType_S32, sprofile::ResultInvalidDataType());

        /* Set the output value. */
        *out = entry.value_s32;
        return ResultSuccess();
    }

    Result ProfileManager::GetUnsigned32(u32 *out, Identifier profile, Identifier key) {
        /* Get the data entry. */
        ProfileDataEntry entry;
        R_TRY(this->GetDataEntry(std::addressof(entry), profile, key));

        /* Check the type. */
        R_UNLESS(entry.type == ValueType_U32, sprofile::ResultInvalidDataType());

        /* Set the output value. */
        *out = entry.value_u32;
        return ResultSuccess();
    }

    Result ProfileManager::GetByte(u8 *out, Identifier profile, Identifier key) {
        /* Get the data entry. */
        ProfileDataEntry entry;
        R_TRY(this->GetDataEntry(std::addressof(entry), profile, key));

        /* Check the type. */
        R_UNLESS(entry.type == ValueType_Byte, sprofile::ResultInvalidDataType());

        /* Set the output value. */
        *out = entry.value_u8;
        return ResultSuccess();
    }

    Result ProfileManager::GetRaw(u8 *out_type, u64 *out_value, Identifier profile, Identifier key) {
        /* Get the data entry. */
        ProfileDataEntry entry;
        R_TRY(this->GetDataEntry(std::addressof(entry), profile, key));

        /* Set the output type and value. */
        *out_type  = entry.type;
        *out_value = entry.value_u64;
        return ResultSuccess();
    }

    Result ProfileManager::CommitImpl() {
        /* Ensure primary directories. */
        R_TRY(this->EnsurePrimaryDirectories());

        /* Declare re-usable paths. */
        char tmp_path[0x30];
        char pri_path[0x30];

        /* Move the metadata. */
        {
            CreateTemporaryMetadataPath(tmp_path, sizeof(tmp_path), m_save_data_info.mount_name);
            CreatePrimaryMetadataPath(pri_path, sizeof(pri_path), m_save_data_info.mount_name);
            R_TRY(MoveFile(tmp_path, pri_path));
        }

        /* Move all profiles. */
        for (auto i = 0; i < m_profile_importer->GetImportingCount(); ++i) {
            const auto id = m_profile_importer->GetImportingProfile(i);

            CreateTemporaryProfilePath(tmp_path, sizeof(tmp_path), m_save_data_info.mount_name, id);
            CreatePrimaryProfilePath(pri_path, sizeof(pri_path), m_save_data_info.mount_name, id);
            R_TRY(MoveFile(tmp_path, pri_path));
        }

        return ResultSuccess();
    }

    void ProfileManager::OnCommitted() {
        /* TODO: Here, Nintendo sets the erpt ServiceProfileRevisionKey to the current revision key. */

        /* If we need to, invalidate the loaded service profile. */
        if (m_service_profile.has_value()) {
            for (auto i = 0; i < m_profile_importer->GetImportingCount(); ++i) {
                if (m_service_profile->name == m_profile_importer->GetImportingProfile(i)) {
                    m_service_profile = util::nullopt;
                    break;
                }
            }
        }

        /* Reset profile metadata. */
        m_profile_metadata = util::nullopt;

        /* Invoke any listeners. */
        for (auto i = 0; i < m_profile_importer->GetImportingCount(); ++i) {
            m_update_observer_manager.OnUpdate(m_profile_importer->GetImportingProfile(i));
        }

        /* Reset profile importer. */
        m_profile_importer = util::nullopt;
    }

    Result ProfileManager::EnsurePrimaryDirectories() {
        /* Ensure the primary directories. */
        char path[0x30];

        CreatePrimaryDirectoryPath(path, sizeof(path), m_save_data_info.mount_name);
        R_TRY(EnsureDirectory(path));

        CreatePrimaryProfileDirectoryPath(path, sizeof(path), m_save_data_info.mount_name);
        R_TRY(EnsureDirectory(path));

        return ResultSuccess();
    }

    Result ProfileManager::EnsureTemporaryDirectories() {
        /* Ensure the temporary directories. */
        char path[0x30];

        CreateTemporaryDirectoryPath(path, sizeof(path), m_save_data_info.mount_name);
        R_TRY(EnsureDirectory(path));

        CreateTemporaryProfileDirectoryPath(path, sizeof(path), m_save_data_info.mount_name);
        R_TRY(EnsureDirectory(path));

        return ResultSuccess();
    }

}
