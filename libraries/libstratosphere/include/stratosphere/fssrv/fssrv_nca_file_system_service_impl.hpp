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
#include <vapours.hpp>
#include <stratosphere/fssystem/fssystem_pimpl.hpp>
#include <stratosphere/fssrv/fssrv_file_system_proxy_api.hpp>

namespace ams::fssrv {

    class BaseFileSystemServiceImpl;
    class ProgramRegistryServiceImpl;
    class AccessFailureManagementServiceImpl;


    namespace fscreator {

        class LocalFileSystemCreator;
        class TargetManagerFileSystemCreator;
        class PartitionFileSystemCreator;
        class IRomFileSystemCreator;
        class StorageOnNcaCreator;
        class SubDirectoryFileSystemCreator;
        class EncryptedFileSystemCreator;
        class INspRootFileSystemCreator;

    }

    namespace impl {

        class UpdatePartitionPath;
        class ExternalKeyManager;
        class LocationResolverSet;
        class SystemDataUpdateEventManager;

    }

    /* ACCURATE_TO_VERSION: Unknown */
    class NcaFileSystemServiceImpl {
        public:
            struct Configuration {
                BaseFileSystemServiceImpl *base_fs_service;
                fscreator::LocalFileSystemCreator *local_fs_creator;
                fscreator::TargetManagerFileSystemCreator *tm_fs_creator;
                fscreator::PartitionFileSystemCreator *partition_fs_creator;
                fscreator::IRomFileSystemCreator *rom_fs_creator;
                fscreator::StorageOnNcaCreator *storage_on_nca_creator;
                fscreator::SubDirectoryFileSystemCreator *subdir_fs_creator;
                fscreator::EncryptedFileSystemCreator *encrypted_fs_creator;
                fscreator::INspRootFileSystemCreator *nsp_root_fs_creator;
                ProgramRegistryServiceImpl *program_registry_service;
                AccessFailureManagementServiceImpl *access_failure_management_service;
                InternalProgramIdRangeForSpeedEmulation program_id_range_for_speed_emulation;
            };
        private:
            struct MountInfo {
                bool is_game_card;
                u32 gc_handle;
                bool is_host_fs;
                bool can_mount_nca;
            };
        private:
            Configuration m_config;
            fssystem::Pimpl<impl::UpdatePartitionPath, 0x350> m_update_partition_path;
            fssystem::Pimpl<impl::ExternalKeyManager, 0x50> m_external_key_manager;
            fssystem::Pimpl<impl::UpdatePartitionPath, 0xA8> m_location_resolver_set;
            fssystem::Pimpl<impl::SystemDataUpdateEventManager, 0x48> m_system_data_update_event_manager;
            fs::EncryptionSeed m_encryption_seed;
            int m_romfs_remount_for_data_corruption_count;
            int m_romfs_unrecoverable_data_corruption_by_remount_count;
            int m_romfs_recovered_by_invalidate_cache_count;
            os::SdkMutex m_romfs_count_mutex;
        public:
            NcaFileSystemServiceImpl(const Configuration &cfg) : m_config(cfg) { /* ... */ }
    };

}
