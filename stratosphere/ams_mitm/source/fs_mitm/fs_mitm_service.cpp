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
#include "../amsmitm_fs_utils.hpp"
#include "../amsmitm_initialization.hpp"
#include "fs_shim.h"
#include "fs_mitm_service.hpp"
#include "fsmitm_boot0storage.hpp"
#include "fsmitm_calibration_binary_storage.hpp"
#include "fsmitm_layered_romfs_storage.hpp"
#include "fsmitm_save_utils.hpp"
#include "fsmitm_readonly_layered_filesystem.hpp"

namespace ams::mitm::fs {

    using namespace ams::fs;

    namespace {

        constexpr const char AtmosphereHblWebContentDir[] = "/atmosphere/hbl_html/";
        constexpr const char ProgramWebContentDir[] = "/manual_html/";

        constinit os::SdkMutex g_data_storage_lock;
        constinit os::SdkMutex g_storage_cache_lock;
        std::unordered_map<u64, std::weak_ptr<ams::fssrv::sf::IStorage>> g_storage_cache;

        constinit os::SdkMutex g_boot0_detect_lock;
        constinit bool g_detected_boot0_kind = false;
        constinit bool g_is_boot0_custom_public_key = false;

        bool IsBoot0CustomPublicKey(::FsStorage &storage) {
            if (AMS_UNLIKELY(!g_detected_boot0_kind)) {
                std::scoped_lock lk(g_boot0_detect_lock);

                if (AMS_LIKELY(!g_detected_boot0_kind)) {
                    g_is_boot0_custom_public_key = DetectBoot0CustomPublicKey(storage);
                    g_detected_boot0_kind = true;
                }
            }

            return g_is_boot0_custom_public_key;
        }

        std::shared_ptr<ams::fssrv::sf::IStorage> GetStorageCacheEntry(ncm::ProgramId program_id) {
            std::scoped_lock lk(g_storage_cache_lock);

            auto it = g_storage_cache.find(static_cast<u64>(program_id));
            if (it == g_storage_cache.end()) {
                return nullptr;
            }

            return it->second.lock();
        }

        void SetStorageCacheEntry(ncm::ProgramId program_id, std::shared_ptr<ams::fssrv::sf::IStorage> *new_intf) {
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
            AMS_ABORT_UNLESS(settings::fwdbg::GetSettingsItemValue(&tmp, sizeof(tmp), name, key) == sizeof(tmp));
            return (tmp != 0);
        }

        template<typename... Arguments>
        constexpr ALWAYS_INLINE auto MakeSharedFileSystem(Arguments &&... args) {
            return sf::MakeShared<ams::fssrv::sf::IFileSystem, ams::fssrv::impl::FileSystemInterfaceAdapter>(std::forward<Arguments>(args)...);
        }

        template<typename... Arguments>
        constexpr ALWAYS_INLINE auto MakeSharedStorage(Arguments &&... args) {
            return sf::MakeShared<ams::fssrv::sf::IStorage, ams::fssrv::impl::StorageInterfaceAdapter>(std::forward<Arguments>(args)...);
        }

        Result OpenHblWebContentFileSystem(sf::Out<std::shared_ptr<ams::fssrv::sf::IFileSystem>> &out, ncm::ProgramId client_program_id, ncm::ProgramId program_id, FsFileSystemType filesystem_type) {
            /* Verify eligibility. */
            bool is_hbl;
            R_UNLESS(R_SUCCEEDED(pm::info::IsHblProgramId(&is_hbl, program_id)), sm::mitm::ResultShouldForwardToSession());
            R_UNLESS(is_hbl,                                                     sm::mitm::ResultShouldForwardToSession());

            /* Hbl html directory must exist. */
            {
                FsDir d;
                R_UNLESS(R_SUCCEEDED(mitm::fs::OpenSdDirectory(&d, AtmosphereHblWebContentDir, fs::OpenDirectoryMode_Directory)), sm::mitm::ResultShouldForwardToSession());
                fsDirClose(&d);
            }

            /* Open the SD card using fs.mitm's session. */
            FsFileSystem sd_fs;
            R_TRY(fsOpenSdCardFileSystem(&sd_fs));
            const sf::cmif::DomainObjectId target_object_id{serviceGetObjectId(&sd_fs.s)};
            std::unique_ptr<fs::fsa::IFileSystem> sd_ifs = std::make_unique<fs::RemoteFileSystem>(sd_fs);

            out.SetValue(MakeSharedFileSystem(std::make_shared<fs::ReadOnlyFileSystem>(std::make_unique<fssystem::SubDirectoryFileSystem>(std::move(sd_ifs), AtmosphereHblWebContentDir)), false), target_object_id);
            return ResultSuccess();
        }

        Result OpenProgramSpecificWebContentFileSystem(sf::Out<std::shared_ptr<ams::fssrv::sf::IFileSystem>> &out, ncm::ProgramId client_program_id, ncm::ProgramId program_id, FsFileSystemType filesystem_type, Service *fwd, const fssrv::sf::Path *path, bool with_id) {
            /* Directory must exist. */
            {
                FsDir d;
                R_UNLESS(R_SUCCEEDED(mitm::fs::OpenAtmosphereSdDirectory(&d, program_id, ProgramWebContentDir, fs::OpenDirectoryMode_Directory)), sm::mitm::ResultShouldForwardToSession());
                fsDirClose(&d);
            }

            /* Open the SD card using fs.mitm's session. */
            FsFileSystem sd_fs;
            R_TRY(fsOpenSdCardFileSystem(&sd_fs));
            const sf::cmif::DomainObjectId target_object_id{serviceGetObjectId(&sd_fs.s)};
            std::unique_ptr<fs::fsa::IFileSystem> sd_ifs = std::make_unique<fs::RemoteFileSystem>(sd_fs);

            /* Format the subdirectory path. */
            char program_web_content_path[fs::EntryNameLengthMax + 1];
            FormatAtmosphereSdPath(program_web_content_path, sizeof(program_web_content_path), program_id, ProgramWebContentDir);

            /* Make a new filesystem. */
            {
                std::unique_ptr<fs::fsa::IFileSystem> subdir_fs = std::make_unique<fssystem::SubDirectoryFileSystem>(std::move(sd_ifs), program_web_content_path);
                std::shared_ptr<fs::fsa::IFileSystem> new_fs = nullptr;

                /* Try to open the existing fs. */
                FsFileSystem base_fs;
                bool opened_base_fs = false;
                if (with_id) {
                    opened_base_fs = R_SUCCEEDED(fsOpenFileSystemWithIdFwd(fwd, std::addressof(base_fs), static_cast<u64>(program_id), filesystem_type, path->str));
                } else {
                    opened_base_fs = R_SUCCEEDED(fsOpenFileSystemWithPatchFwd(fwd, std::addressof(base_fs), static_cast<u64>(program_id), filesystem_type));
                }

                if (opened_base_fs) {
                    /* Create a layered adapter. */
                    new_fs = std::make_shared<ReadOnlyLayeredFileSystem>(std::move(subdir_fs), std::make_unique<fs::RemoteFileSystem>(base_fs));
                } else {
                    /* Without an existing FS, just make a read only adapter to the subdirectory. */
                    new_fs = std::make_shared<fs::ReadOnlyFileSystem>(std::move(subdir_fs));
                }

                out.SetValue(MakeSharedFileSystem(std::move(new_fs), false), target_object_id);
            }

            return ResultSuccess();
        }

        Result OpenWebContentFileSystem(sf::Out<std::shared_ptr<ams::fssrv::sf::IFileSystem>> &out, ncm::ProgramId client_program_id, ncm::ProgramId program_id, FsFileSystemType filesystem_type, Service *fwd, const fssrv::sf::Path *path, bool with_id, bool try_program_specific) {
            /* Check first that we're a web applet opening web content. */
            R_UNLESS(ncm::IsWebAppletId(client_program_id),             sm::mitm::ResultShouldForwardToSession());
            R_UNLESS(filesystem_type == FsFileSystemType_ContentManual, sm::mitm::ResultShouldForwardToSession());

            /* Try to mount the HBL web filesystem. If this succeeds then we're done. */
            R_UNLESS(R_FAILED(OpenHblWebContentFileSystem(out, client_program_id, program_id, filesystem_type)), ResultSuccess());

            /* If program specific override shouldn't be attempted, fall back. */
            R_UNLESS(try_program_specific, sm::mitm::ResultShouldForwardToSession());

            /* If we're not opening a HBL filesystem, just try to open a generic one. */
            return OpenProgramSpecificWebContentFileSystem(out, client_program_id, program_id, filesystem_type, fwd, path, with_id);
        }

    }

    Result FsMitmService::OpenFileSystemWithPatch(sf::Out<std::shared_ptr<ams::fssrv::sf::IFileSystem>> out, ncm::ProgramId program_id, u32 _filesystem_type) {
        return OpenWebContentFileSystem(out, this->client_info.program_id, program_id, static_cast<FsFileSystemType>(_filesystem_type), this->forward_service.get(), nullptr, false, this->client_info.override_status.IsProgramSpecific());
    }

    Result FsMitmService::OpenFileSystemWithId(sf::Out<std::shared_ptr<ams::fssrv::sf::IFileSystem>> out, const fssrv::sf::Path &path, ncm::ProgramId program_id, u32 _filesystem_type) {
        return OpenWebContentFileSystem(out, this->client_info.program_id, program_id, static_cast<FsFileSystemType>(_filesystem_type), this->forward_service.get(), std::addressof(path), true, this->client_info.override_status.IsProgramSpecific());
    }

    Result FsMitmService::OpenSdCardFileSystem(sf::Out<std::shared_ptr<ams::fssrv::sf::IFileSystem>> out) {
        /* We only care about redirecting this for NS/emummc. */
        R_UNLESS(this->client_info.program_id == ncm::SystemProgramId::Ns, sm::mitm::ResultShouldForwardToSession());
        R_UNLESS(emummc::IsActive(),                                       sm::mitm::ResultShouldForwardToSession());

        /* Create a new SD card filesystem. */
        FsFileSystem sd_fs;
        R_TRY(fsOpenSdCardFileSystemFwd(this->forward_service.get(), &sd_fs));
        const sf::cmif::DomainObjectId target_object_id{serviceGetObjectId(&sd_fs.s)};

        /* Return output filesystem. */
        std::shared_ptr<fs::fsa::IFileSystem> redir_fs = std::make_shared<fssystem::DirectoryRedirectionFileSystem>(std::make_shared<RemoteFileSystem>(sd_fs), "/Nintendo", emummc::GetNintendoDirPath());
        out.SetValue(MakeSharedFileSystem(std::move(redir_fs), false), target_object_id);
        return ResultSuccess();
    }

    Result FsMitmService::OpenSaveDataFileSystem(sf::Out<std::shared_ptr<ams::fssrv::sf::IFileSystem>> out, u8 _space_id, const fs::SaveDataAttribute &attribute) {
        /* We only want to intercept saves for games, right now. */
        const bool is_game_or_hbl = this->client_info.override_status.IsHbl() || ncm::IsApplicationId(this->client_info.program_id);
        R_UNLESS(is_game_or_hbl, sm::mitm::ResultShouldForwardToSession());

        /* Only redirect if the appropriate system setting is set. */
        R_UNLESS(GetSettingsItemBooleanValue("atmosphere", "fsmitm_redirect_saves_to_sd"), sm::mitm::ResultShouldForwardToSession());

        /* Only redirect if the specific title being accessed has a redirect save flag. */
        R_UNLESS(cfg::HasContentSpecificFlag(this->client_info.program_id, "redirect_save"), sm::mitm::ResultShouldForwardToSession());

        /* Only redirect account savedata. */
        R_UNLESS(attribute.type == fs::SaveDataType::Account, sm::mitm::ResultShouldForwardToSession());

        /* Get enum type for space id. */
        auto space_id = static_cast<FsSaveDataSpaceId>(_space_id);

        /* Verify we can open the save. */
        static_assert(sizeof(fs::SaveDataAttribute) == sizeof(::FsSaveDataAttribute));
        FsFileSystem save_fs;
        R_UNLESS(R_SUCCEEDED(fsOpenSaveDataFileSystemFwd(this->forward_service.get(), &save_fs, space_id, reinterpret_cast<const FsSaveDataAttribute *>(&attribute))), sm::mitm::ResultShouldForwardToSession());
        std::unique_ptr<fs::fsa::IFileSystem> save_ifs = std::make_unique<fs::RemoteFileSystem>(save_fs);

        /* Mount the SD card using fs.mitm's session. */
        FsFileSystem sd_fs;
        R_TRY(fsOpenSdCardFileSystem(&sd_fs));
        const sf::cmif::DomainObjectId target_object_id{serviceGetObjectId(&sd_fs.s)};
        std::shared_ptr<fs::fsa::IFileSystem> sd_ifs = std::make_shared<fs::RemoteFileSystem>(sd_fs);

        /* Verify that we can open the save directory, and that it exists. */
        const ncm::ProgramId application_id = attribute.program_id == ncm::InvalidProgramId ? this->client_info.program_id : attribute.program_id;
        char save_dir_path[fs::EntryNameLengthMax + 1];
        R_TRY(mitm::fs::SaveUtil::GetDirectorySaveDataPath(save_dir_path, sizeof(save_dir_path), application_id, space_id, attribute));

        /* Check if this is the first time we're making the save. */
        bool is_new_save = false;
        {
            fs::DirectoryEntryType ent;
            R_TRY_CATCH(sd_ifs->GetEntryType(&ent, save_dir_path)) {
                R_CATCH(fs::ResultPathNotFound) { is_new_save = true; }
                R_CATCH_ALL() { /* ... */ }
            } R_END_TRY_CATCH;
        }

        /* Ensure the directory exists. */
        R_TRY(fssystem::EnsureDirectoryRecursively(sd_ifs.get(), save_dir_path));

        /* Create directory savedata filesystem. */
        std::unique_ptr<fs::fsa::IFileSystem> subdir_fs = std::make_unique<fssystem::SubDirectoryFileSystem>(sd_ifs, save_dir_path);
        std::shared_ptr<fssystem::DirectorySaveDataFileSystem> dirsave_ifs = std::make_shared<fssystem::DirectorySaveDataFileSystem>(std::move(subdir_fs));

        /* Ensure correct directory savedata filesystem state. */
        R_TRY(dirsave_ifs->Initialize());

        /* If it's the first time we're making the save, copy existing savedata over. */
        if (is_new_save) {
            /* TODO: Check error? */
            dirsave_ifs->CopySaveFromFileSystem(save_ifs.get());
        }

        /* Set output. */
        out.SetValue(MakeSharedFileSystem(std::move(dirsave_ifs), false), target_object_id);
        return ResultSuccess();
    }

    Result FsMitmService::OpenBisStorage(sf::Out<std::shared_ptr<ams::fssrv::sf::IStorage>> out, u32 _bis_partition_id) {
        const ::FsBisPartitionId bis_partition_id = static_cast<::FsBisPartitionId>(_bis_partition_id);

        /* Try to open a storage for the partition. */
        FsStorage bis_storage;
        R_TRY(fsOpenBisStorageFwd(this->forward_service.get(), &bis_storage, bis_partition_id));
        const sf::cmif::DomainObjectId target_object_id{serviceGetObjectId(&bis_storage.s)};

        const bool is_sysmodule = ncm::IsSystemProgramId(this->client_info.program_id);
        const bool is_hbl = this->client_info.override_status.IsHbl();
        const bool can_write_bis = is_sysmodule || (is_hbl && GetSettingsItemBooleanValue("atmosphere", "enable_hbl_bis_write"));

        /* Allow HBL to write to boot1 (safe firm) + package2. */
        /* This is needed to not break compatibility with ChoiDujourNX, which does not check for write access before beginning an update. */
        /* TODO: get fixed so that this can be turned off without causing bricks :/ */
        const bool is_package2 = (FsBisPartitionId_BootConfigAndPackage2Part1 <= bis_partition_id && bis_partition_id <= FsBisPartitionId_BootConfigAndPackage2Part6);
        const bool is_boot1    = bis_partition_id == FsBisPartitionId_BootPartition2Root;
        const bool can_write_bis_for_choi_support = is_hbl && (is_package2 || is_boot1);

        /* Set output storage. */
        if (bis_partition_id == FsBisPartitionId_BootPartition1Root) {
            if (IsBoot0CustomPublicKey(bis_storage)) {
                out.SetValue(MakeSharedStorage(new CustomPublicKeyBoot0Storage(bis_storage, this->client_info, spl::GetSocType())), target_object_id);
            } else {
                out.SetValue(MakeSharedStorage(new Boot0Storage(bis_storage, this->client_info)), target_object_id);
            }
        } else if (bis_partition_id == FsBisPartitionId_CalibrationBinary) {
            out.SetValue(MakeSharedStorage(new CalibrationBinaryStorage(bis_storage, this->client_info)), target_object_id);
        } else {
            if (can_write_bis || can_write_bis_for_choi_support) {
                /* We can write, so create a writable storage. */
                out.SetValue(MakeSharedStorage(new RemoteStorage(bis_storage)), target_object_id);
            } else {
                /* We can only read, so create a readable storage. */
                out.SetValue(MakeSharedStorage(new ReadOnlyStorageAdapter(new RemoteStorage(bis_storage))), target_object_id);
            }
        }

        return ResultSuccess();
    }

    Result FsMitmService::OpenDataStorageByCurrentProcess(sf::Out<std::shared_ptr<ams::fssrv::sf::IStorage>> out) {
        /* Only mitm if we should override contents for the current process. */
        R_UNLESS(this->client_info.override_status.IsProgramSpecific(),     sm::mitm::ResultShouldForwardToSession());

        /* Only mitm if there is actually an override romfs. */
        R_UNLESS(mitm::fs::HasSdRomfsContent(this->client_info.program_id), sm::mitm::ResultShouldForwardToSession());

        /* Try to open the process romfs. */
        FsStorage data_storage;
        R_TRY(fsOpenDataStorageByCurrentProcessFwd(this->forward_service.get(), &data_storage));
        const sf::cmif::DomainObjectId target_object_id{serviceGetObjectId(&data_storage.s)};

        /* Get a scoped lock. */
        std::scoped_lock lk(g_data_storage_lock);

        /* Try to get a storage from the cache. */
        {
            std::shared_ptr<ams::fssrv::sf::IStorage> cached_storage = GetStorageCacheEntry(this->client_info.program_id);
            if (cached_storage != nullptr) {
                out.SetValue(std::move(cached_storage), target_object_id);
                return ResultSuccess();
            }
        }

        /* Make a new layered romfs, and cache to storage. */
        {
            std::shared_ptr<ams::fssrv::sf::IStorage> new_storage_intf = nullptr;

            /* Create the layered storage. */
            FsFile data_file;
            if (R_SUCCEEDED(OpenAtmosphereSdFile(&data_file, this->client_info.program_id, "romfs.bin", OpenMode_Read))) {
                auto layered_storage = std::make_shared<LayeredRomfsStorage>(std::make_unique<ReadOnlyStorageAdapter>(new RemoteStorage(data_storage)), std::make_unique<ReadOnlyStorageAdapter>(new FileStorage(new RemoteFile(data_file))), this->client_info.program_id);
                layered_storage->BeginInitialize();
                new_storage_intf = MakeSharedStorage(layered_storage);
            } else {
                auto layered_storage = std::make_shared<LayeredRomfsStorage>(std::make_unique<ReadOnlyStorageAdapter>(new RemoteStorage(data_storage)), nullptr, this->client_info.program_id);
                layered_storage->BeginInitialize();
                new_storage_intf = MakeSharedStorage(layered_storage);
            }

            SetStorageCacheEntry(this->client_info.program_id, &new_storage_intf);
            out.SetValue(std::move(new_storage_intf), target_object_id);
        }

        return ResultSuccess();
    }

    Result FsMitmService::OpenDataStorageByDataId(sf::Out<std::shared_ptr<ams::fssrv::sf::IStorage>> out, ncm::DataId _data_id, u8 storage_id) {
        /* Only mitm if we should override contents for the current process. */
        R_UNLESS(this->client_info.override_status.IsProgramSpecific(), sm::mitm::ResultShouldForwardToSession());

        /* TODO: Decide how to handle DataId vs ProgramId for this API. */
        const ncm::ProgramId data_id = {_data_id.value};

        /* Only mitm if there is actually an override romfs. */
        R_UNLESS(mitm::fs::HasSdRomfsContent(data_id),                  sm::mitm::ResultShouldForwardToSession());

        /* Try to open the process romfs. */
        FsStorage data_storage;
        R_TRY(fsOpenDataStorageByDataIdFwd(this->forward_service.get(), &data_storage, static_cast<u64>(data_id), static_cast<NcmStorageId>(storage_id)));
        const sf::cmif::DomainObjectId target_object_id{serviceGetObjectId(&data_storage.s)};

        /* Get a scoped lock. */
        std::scoped_lock lk(g_data_storage_lock);

        /* Try to get a storage from the cache. */
        {
            std::shared_ptr<ams::fssrv::sf::IStorage> cached_storage = GetStorageCacheEntry(data_id);
            if (cached_storage != nullptr) {
                out.SetValue(std::move(cached_storage), target_object_id);
                return ResultSuccess();
            }
        }

        /* Make a new layered romfs, and cache to storage. */
        {
            std::shared_ptr<ams::fssrv::sf::IStorage> new_storage_intf = nullptr;

            /* Create the layered storage. */
            FsFile data_file;
            if (R_SUCCEEDED(OpenAtmosphereSdFile(&data_file, data_id, "romfs.bin", OpenMode_Read))) {
                auto layered_storage = std::make_shared<LayeredRomfsStorage>(std::make_unique<ReadOnlyStorageAdapter>(new RemoteStorage(data_storage)), std::make_unique<ReadOnlyStorageAdapter>(new FileStorage(new RemoteFile(data_file))), data_id);
                layered_storage->BeginInitialize();
                new_storage_intf = MakeSharedStorage(layered_storage);
            } else {
                auto layered_storage = std::make_shared<LayeredRomfsStorage>(std::make_unique<ReadOnlyStorageAdapter>(new RemoteStorage(data_storage)), nullptr, data_id);
                layered_storage->BeginInitialize();
                new_storage_intf = MakeSharedStorage(layered_storage);
            }

            SetStorageCacheEntry(data_id, &new_storage_intf);
            out.SetValue(std::move(new_storage_intf), target_object_id);
        }

        return ResultSuccess();
    }

}
