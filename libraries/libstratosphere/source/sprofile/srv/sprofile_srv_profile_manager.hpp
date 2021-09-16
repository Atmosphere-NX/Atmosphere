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
#pragma once
#include <stratosphere.hpp>

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
            class UpdateObserverManager;
        private:
            os::SdkMutex m_general_mutex;
            os::SdkMutex m_fs_mutex;
            SaveDataInfo m_save_data_info;
            /* TODO: util::optional<ProfileImporter> m_profile_importer; */
            /* TODO: util::optional<ProfileMetadata> m_profile_metadata; */
            /* TODO: util::optional<ServiceProfile> m_service_profile; */
            UpdateObserverManager *m_update_observer_manager;
        public:
            ProfileManager(const SaveDataInfo &save_data_info);
        public:
            /* TODO */
        private:
            /* TODO */
    };

}
