/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "../amsmitm_fs_utils.hpp"
#include "fs_shim.h"
#include "fs_mitm_service.hpp"
#include "fsmitm_boot0storage.hpp"
#include "fsmitm_layered_romfs_storage.hpp"

namespace ams::mitm::fs {

    using namespace ams::fs;

    namespace {

        os::Mutex g_storage_cache_lock;
        std::unordered_map<u64, std::weak_ptr<IStorageInterface>> g_storage_cache;

        std::shared_ptr<IStorageInterface> GetStorageCacheEntry(ncm::ProgramId program_id) {
            std::scoped_lock lk(g_storage_cache_lock);

            auto it = g_storage_cache.find(static_cast<u64>(program_id));
            if (it == g_storage_cache.end()) {
                return nullptr;
            }

            return it->second.lock();
        }

        void SetStorageCacheEntry(ncm::ProgramId program_id, std::shared_ptr<IStorageInterface> *new_intf) {
            std::scoped_lock lk(g_storage_cache_lock);

            auto it = g_storage_cache.find(static_cast<u64>(program_id));
            if (it != g_storage_cache.end()) {
                auto cur_intf = it->second.lock();
                if (cur_intf != nullptr) {
                    *new_intf = cur_intf;
                    return;
                }
            }

            g_storage_cache[static_cast<u64>(program_id)] = *new_intf;
        }

        bool GetSettingsItemBooleanValue(const char *name, const char *key) {
            u8 tmp = 0;
            AMS_ASSERT(settings::fwdbg::GetSettingsItemValue(&tmp, sizeof(tmp), name, key) == sizeof(tmp));
            return (tmp != 0);
        }

    }

    Result FsMitmService::OpenSdCardFileSystem(sf::Out<std::shared_ptr<IFileSystemInterface>> out) {
        /* We only care about redirecting this for NS/emummc. */
        R_UNLESS(this->client_info.program_id == ncm::ProgramId::Ns, sm::mitm::ResultShouldForwardToSession());
        R_UNLESS(emummc::IsActive(),                                 sm::mitm::ResultShouldForwardToSession());

        /* Create a new SD card filesystem. */
        FsFileSystem sd_fs;
        R_TRY(fsOpenSdCardFileSystemFwd(this->forward_service.get(), &sd_fs));
        const sf::cmif::DomainObjectId target_object_id{serviceGetObjectId(&sd_fs.s)};

        /* Return output filesystem. */
        std::shared_ptr<fs::fsa::IFileSystem> redir_fs = std::make_shared<fssystem::DirectoryRedirectionFileSystem>(std::make_shared<RemoteFileSystem>(sd_fs), "/Nintendo", emummc::GetNintendoDirPath());
        out.SetValue(std::make_shared<IFileSystemInterface>(std::move(redir_fs), false), target_object_id);
        return ResultSuccess();
    }

    Result FsMitmService::OpenBisStorage(sf::Out<std::shared_ptr<IStorageInterface>> out, u32 _bis_partition_id) {
        const ::FsBisPartitionId bis_partition_id = static_cast<::FsBisPartitionId>(_bis_partition_id);

        /* Try to open a storage for the partition. */
        FsStorage bis_storage;
        R_TRY(fsOpenBisStorageFwd(this->forward_service.get(), &bis_storage, bis_partition_id));
        const sf::cmif::DomainObjectId target_object_id{serviceGetObjectId(&bis_storage.s)};

        const bool is_sysmodule = ncm::IsSystemProgramId(this->client_info.program_id);
        const bool is_hbl = this->client_info.override_status.IsHbl();
        const bool can_write_bis = is_sysmodule || (is_hbl && GetSettingsItemBooleanValue("atmosphere", "enable_hbl_bis_write"));
        const bool can_read_cal  = is_sysmodule || (is_hbl && GetSettingsItemBooleanValue("atmosphere", "enable_hbl_cal_read"));

        /* Allow HBL to write to boot1 (safe firm) + package2. */
        /* This is needed to not break compatibility with ChoiDujourNX, which does not check for write access before beginning an update. */
        /* TODO: get fixed so that this can be turned off without causing bricks :/ */
        const bool is_package2 = (FsBisPartitionId_BootConfigAndPackage2Part1 <= bis_partition_id && bis_partition_id <= FsBisPartitionId_BootConfigAndPackage2Part6);
        const bool is_boot1    = bis_partition_id == FsBisPartitionId_BootPartition2Root;
        const bool can_write_bis_for_choi_support = is_hbl && (is_package2 || is_boot1);

        /* Set output storage. */
        if (bis_partition_id == FsBisPartitionId_BootPartition1Root) {
            out.SetValue(std::make_shared<IStorageInterface>(new Boot0Storage(bis_storage, this->client_info)), target_object_id);
        } else if (bis_partition_id == FsBisPartitionId_CalibrationBinary) {
            /* PRODINFO should *never* be writable. */
            /* If we have permissions, create a read only storage. */
            if (can_read_cal) {
                out.SetValue(std::make_shared<IStorageInterface>(new ReadOnlyStorageAdapter(new RemoteStorage(bis_storage))), target_object_id);
            } else {
                /* If we can't read cal, return permission denied. */
                fsStorageClose(&bis_storage);
                return fs::ResultPermissionDenied();
            }
        } else {
            if (can_write_bis || can_write_bis_for_choi_support) {
                /* We can write, so create a writable storage. */
                out.SetValue(std::make_shared<IStorageInterface>(new RemoteStorage(bis_storage)), target_object_id);
            } else {
                /* We can only read, so create a readable storage. */
                out.SetValue(std::make_shared<IStorageInterface>(new ReadOnlyStorageAdapter(new RemoteStorage(bis_storage))), target_object_id);
            }
        }

        return ResultSuccess();
    }

    Result FsMitmService::OpenDataStorageByCurrentProcess(sf::Out<std::shared_ptr<IStorageInterface>> out) {
        /* Only mitm if we should override contents for the current process. */
        R_UNLESS(this->client_info.override_status.IsProgramSpecific(),     sm::mitm::ResultShouldForwardToSession());

        /* Only mitm if there is actually an override romfs. */
        R_UNLESS(mitm::fs::HasSdRomfsContent(this->client_info.program_id), sm::mitm::ResultShouldForwardToSession());

        /* Try to open the process romfs. */
        FsStorage data_storage;
        R_TRY(fsOpenDataStorageByCurrentProcessFwd(this->forward_service.get(), &data_storage));
        const sf::cmif::DomainObjectId target_object_id{serviceGetObjectId(&data_storage.s)};

        /* Try to get a storage from the cache. */
        {
            std::shared_ptr<IStorageInterface> cached_storage = GetStorageCacheEntry(this->client_info.program_id);
            if (cached_storage != nullptr) {
                out.SetValue(std::move(cached_storage), target_object_id);
                return ResultSuccess();
            }
        }

        /* Make a new layered romfs, and cache to storage. */
        {
            std::shared_ptr<IStorageInterface> new_storage_intf = nullptr;

            /* Create the layered storage. */
            FsFile data_file;
            if (R_SUCCEEDED(OpenAtmosphereSdFile(&data_file, this->client_info.program_id, "romfs.bin", OpenMode_Read))) {
                auto *layered_storage = new LayeredRomfsStorage(std::make_unique<ReadOnlyStorageAdapter>(new RemoteStorage(data_storage)), std::make_unique<ReadOnlyStorageAdapter>(new FileStorage(new RemoteFile(data_file))), this->client_info.program_id);
                new_storage_intf = std::make_shared<IStorageInterface>(layered_storage);
            } else {
                auto *layered_storage = new LayeredRomfsStorage(std::make_unique<ReadOnlyStorageAdapter>(new RemoteStorage(data_storage)), nullptr, this->client_info.program_id);
                new_storage_intf = std::make_shared<IStorageInterface>(layered_storage);
            }

            SetStorageCacheEntry(this->client_info.program_id, &new_storage_intf);
            out.SetValue(std::move(new_storage_intf), target_object_id);
        }

        return ResultSuccess();
    }

    Result FsMitmService::OpenDataStorageByDataId(sf::Out<std::shared_ptr<IStorageInterface>> out, ncm::ProgramId /* TODO: ncm::DataId */ data_id, u8 storage_id) {
        /* Only mitm if we should override contents for the current process. */
        R_UNLESS(this->client_info.override_status.IsProgramSpecific(), sm::mitm::ResultShouldForwardToSession());

        /* Only mitm if there is actually an override romfs. */
        R_UNLESS(mitm::fs::HasSdRomfsContent(data_id),                  sm::mitm::ResultShouldForwardToSession());

        /* Try to open the process romfs. */
        FsStorage data_storage;
        R_TRY(fsOpenDataStorageByDataIdFwd(this->forward_service.get(), &data_storage, static_cast<u64>(data_id), static_cast<NcmStorageId>(storage_id)));
        const sf::cmif::DomainObjectId target_object_id{serviceGetObjectId(&data_storage.s)};

        /* Try to get a storage from the cache. */
        {
            std::shared_ptr<IStorageInterface> cached_storage = GetStorageCacheEntry(data_id);
            if (cached_storage != nullptr) {
                out.SetValue(std::move(cached_storage), target_object_id);
                return ResultSuccess();
            }
        }

        /* Make a new layered romfs, and cache to storage. */
        {
            std::shared_ptr<IStorageInterface> new_storage_intf = nullptr;

            /* Create the layered storage. */
            FsFile data_file;
            if (R_SUCCEEDED(OpenAtmosphereSdFile(&data_file, data_id, "romfs.bin", OpenMode_Read))) {
                auto *layered_storage = new LayeredRomfsStorage(std::make_unique<ReadOnlyStorageAdapter>(new RemoteStorage(data_storage)), std::make_unique<ReadOnlyStorageAdapter>(new FileStorage(new RemoteFile(data_file))), data_id);
                new_storage_intf = std::make_shared<IStorageInterface>(layered_storage);
            } else {
                auto *layered_storage = new LayeredRomfsStorage(std::make_unique<ReadOnlyStorageAdapter>(new RemoteStorage(data_storage)), nullptr, data_id);
                new_storage_intf = std::make_shared<IStorageInterface>(layered_storage);
            }

            SetStorageCacheEntry(data_id, &new_storage_intf);
            out.SetValue(std::move(new_storage_intf), target_object_id);
        }

        return ResultSuccess();
    }

}
