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
        : m_general_mutex(), m_fs_mutex(), m_save_data_info(save_data_info), m_save_file_mounted(false),
          m_update_observer_manager()
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

}
