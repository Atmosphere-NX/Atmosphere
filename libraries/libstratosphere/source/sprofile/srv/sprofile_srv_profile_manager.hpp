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
#pragma once
#include <stratosphere.hpp>
#include "sprofile_srv_types.hpp"
#include "sprofile_srv_profile_update_observer_impl.hpp"
#include "sprofile_srv_profile_importer.hpp"

namespace ams::sprofile::srv {

    class ProfileManager {
        public:
            struct SaveDataInfo {
                u64 id;
                const char *mount_name;
                size_t size;
                size_t journal_size;
                u32 flags;
            };
        private:
        private:
            os::SdkMutex m_general_mutex{};
            os::SdkMutex m_fs_mutex{};
            SaveDataInfo m_save_data_info;
            bool m_save_file_mounted;
            util::optional<ProfileImporter> m_profile_importer;
            os::SdkMutex m_profile_importer_mutex{};
            util::optional<ProfileMetadata> m_profile_metadata;
            os::SdkMutex m_profile_metadata_mutex{};
            util::optional<ServiceProfile> m_service_profile;
            os::SdkMutex m_service_profile_mutex{};
            ProfileUpdateObserverManager m_update_observer_manager;
        public:
            ProfileManager(const SaveDataInfo &save_data_info);
        public:
            void InitializeSaveData();
            Result ResetSaveData();

            Result OpenProfileImporter();
            void CloseProfileImporter();

            Result ImportProfile(const sprofile::srv::ProfilePayload &data);
            Result Commit();
            Result ImportMetadata(const sprofile::srv::MetadataPayload &data);

            Result LoadPrimaryMetadata(ProfileMetadata *out);

            Result GetInt64Value(s64 *out, HashKey profile, HashKey key);
            Result GetUInt64Value(u64 *out, HashKey profile, HashKey key);
            Result GetInt32Value(s32 *out, HashKey profile, HashKey key);
            Result GetUInt32Value(u32 *out, HashKey profile, HashKey key);
            Result GetBooleanValue(u8 *out, HashKey profile, HashKey key);

            Result QueryValue(u8 *out_type, u64 *out_value, HashKey profile, HashKey key);

            ProfileUpdateObserverManager &GetUpdateObserverManager() { return m_update_observer_manager; }
        private:
            Result CommitImportedProfiles();
            Result CleanupOrphanedProfiles();

            Result LoadPrimaryMetadataImpl();
            void CloseProfileImporterImpl();

            void OnCommitted();

            Result GetDataEntry(ProfileDataEntry *out, HashKey profile, HashKey key);

            Result LoadProfile(HashKey profile);

            Result EnsurePrimaryDirectories();
            Result EnsureTemporaryDirectories();
    };

}
