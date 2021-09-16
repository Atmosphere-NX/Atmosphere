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
#include "sprofile_srv_profile_manager.hpp"
#include "sprofile_srv_fs_utils.hpp"

namespace ams::sprofile::srv {

    namespace {

        Result CreateSaveData(const ProfileManager::SaveDataInfo &save_data_info) {
            R_TRY_CATCH(fs::CreateSystemSaveData(save_data_info.id, save_data_info.size, save_data_info.journal_size, save_data_info.flags)) {
                R_CATCH(fs::ResultPathAlreadyExists) { /* Nintendo accepts already-existing savedata here. */ }
            } R_END_TRY_CATCH;
            return ResultSuccess();
        }

    }

    ProfileManager::ProfileManager(const SaveDataInfo &save_data_info)
        : m_save_data_info(save_data_info), m_save_file_mounted(false),
          m_profile_metadata(util::nullopt), m_update_observer_manager()
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

            /* TODO: Remove this after implementation, as it's for debugging. */
            R_ABORT_UNLESS(fs::MountSdCard("sprof-dbg"));
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
        /* TODO m_service_profile = util::nullopt; */

        /* Create the save data. */
        R_TRY(CreateSaveData(m_save_data_info));

        /* Try to mount the save file. */
        const auto result = fs::MountSystemSaveData(m_save_data_info.mount_name, m_save_data_info.id);
        m_save_file_mounted = R_SUCCEEDED(result);

        return result;
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
            AMS_ABORT_UNLESS(util::TSNPrintf(path, sizeof(path), "%s:/%s/metadata", m_save_data_info.mount_name, "primary") < static_cast<int>(sizeof(path)));
            R_TRY(ReadFile(path, std::addressof(*m_profile_metadata), sizeof(*m_profile_metadata), 0));

            /* We read the metadata successfully. */
            meta_guard.Cancel();
        }

        /* Set the output. */
        *out = *m_profile_metadata;
        return ResultSuccess();
    }

}
