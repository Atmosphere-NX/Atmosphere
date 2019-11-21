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

#include <map>
#include <memory>
#include <mutex>

#include <switch.h>
#include <stratosphere.hpp>
#include "fsmitm_service.hpp"
#include "fs_shim.h"

#include "../utils.hpp"
#include "fsmitm_boot0storage.hpp"
#include "fsmitm_layeredrom.hpp"

#include "fs_dir_utils.hpp"
#include "fs_save_utils.hpp"
#include "fs_file_storage.hpp"
#include "fs_subdirectory_filesystem.hpp"
#include "fs_directory_redirection_filesystem.hpp"
#include "fs_directory_savedata_filesystem.hpp"

#include "../debug.hpp"

static ams::os::Mutex g_StorageCacheLock;
static std::unordered_map<u64, std::weak_ptr<IStorageInterface>> g_StorageCache;

static bool StorageCacheGetEntry(ams::ncm::ProgramId program_id, std::shared_ptr<IStorageInterface> *out) {
    std::scoped_lock lock(g_StorageCacheLock);
    if (g_StorageCache.find(static_cast<u64>(program_id)) == g_StorageCache.end()) {
        return false;
    }

    auto intf = g_StorageCache[static_cast<u64>(program_id)].lock();
    if (intf != nullptr) {
        *out = intf;
        return true;
    }
    return false;
}

static void StorageCacheSetEntry(ams::ncm::ProgramId program_id, std::shared_ptr<IStorageInterface> *ptr) {
    std::scoped_lock lock(g_StorageCacheLock);

    /* Ensure we always use the cached copy if present. */
    if (g_StorageCache.find(static_cast<u64>(program_id)) != g_StorageCache.end()) {
        auto intf = g_StorageCache[static_cast<u64>(program_id)].lock();
        if (intf != nullptr) {
            *ptr = intf;
        }
    }

    g_StorageCache[static_cast<u64>(program_id)] = *ptr;
}

void FsMitmService::PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx) {
    auto this_ptr = static_cast<FsMitmService *>(obj);
    switch (static_cast<CommandId>(ctx->cmd_id)) {
        case CommandId::SetCurrentProcess:
            if (R_SUCCEEDED(ctx->rc)) {
                this_ptr->has_initialized = true;
            }
            break;
        default:
            break;
    }
}

Result FsMitmService::OpenHblWebContentFileSystem(Out<std::shared_ptr<IFileSystemInterface>> &out_fs) {
    /* Mount the SD card using fs.mitm's session. */
    FsFileSystem sd_fs;
    R_TRY(fsMountSdcard(&sd_fs));

    /* Set output filesystem. */
    std::unique_ptr<IFileSystem> web_ifs = std::make_unique<SubDirectoryFileSystem>(std::make_shared<ProxyFileSystem>(sd_fs), AtmosphereHblWebContentDir);
    out_fs.SetValue(std::make_shared<IFileSystemInterface>(std::move(web_ifs)));
    if (out_fs.IsDomain()) {
        out_fs.ChangeObjectId(sd_fs.s.object_id);
    }

    return ResultSuccess();
}

Result FsMitmService::OpenFileSystemWithPatch(Out<std::shared_ptr<IFileSystemInterface>> out_fs, u64 program_id, u32 filesystem_type) {
    /* Check for eligibility. */
    {
        FsDir d;
        if (!Utils::IsWebAppletTid(static_cast<u64>(this->program_id)) || filesystem_type != FsFileSystemType_ContentManual || !Utils::IsHblTid(program_id) ||
            R_FAILED(Utils::OpenSdDir(AtmosphereHblWebContentDir, &d))) {
            return ResultAtmosphereMitmShouldForwardToSession;
        }
        fsDirClose(&d);
    }

    /* If there's an existing filesystem, don't override. */
    /* TODO: Multiplex, overriding existing content with HBL content. */
    {
        FsFileSystem fs;
        if (R_SUCCEEDED(fsOpenFileSystemWithPatchFwd(this->forward_service.get(), &fs, program_id, static_cast<FsFileSystemType>(filesystem_type)))) {
            fsFsClose(&fs);
            return ResultAtmosphereMitmShouldForwardToSession;
        }
    }

    return this->OpenHblWebContentFileSystem(out_fs);
}

Result FsMitmService::OpenFileSystemWithId(Out<std::shared_ptr<IFileSystemInterface>> out_fs, InPointer<char> path, u64 program_id, u32 filesystem_type) {
    /* Check for eligibility. */
    {
        FsDir d;
        if (!Utils::IsWebAppletTid(static_cast<u64>(this->program_id)) || filesystem_type != FsFileSystemType_ContentManual || !Utils::IsHblTid(program_id) ||
            R_FAILED(Utils::OpenSdDir(AtmosphereHblWebContentDir, &d))) {
            return ResultAtmosphereMitmShouldForwardToSession;
        }
        fsDirClose(&d);
    }

    /* If there's an existing filesystem, don't override. */
    /* TODO: Multiplex, overriding existing content with HBL content. */
    {
        FsFileSystem fs;
        if (R_SUCCEEDED(fsOpenFileSystemWithIdFwd(this->forward_service.get(), &fs, program_id, static_cast<FsFileSystemType>(filesystem_type), path.pointer))) {
            fsFsClose(&fs);
            return ResultAtmosphereMitmShouldForwardToSession;
        }
    }

    return this->OpenHblWebContentFileSystem(out_fs);
}

Result FsMitmService::OpenSdCardFileSystem(Out<std::shared_ptr<IFileSystemInterface>> out_fs) {
    /* We only care about redirecting this for NS/Emummc. */
    if (this->program_id != ams::ncm::ProgramId::Ns) {
        return ResultAtmosphereMitmShouldForwardToSession;
    }
    if (!IsEmummc()) {
        return ResultAtmosphereMitmShouldForwardToSession;
    }

    /* Mount the SD card. */
    FsFileSystem sd_fs;
    R_TRY(fsMountSdcard(&sd_fs));

    /* Set output filesystem. */
    std::unique_ptr<IFileSystem> redir_fs = std::make_unique<DirectoryRedirectionFileSystem>(new ProxyFileSystem(sd_fs), "/Nintendo", GetEmummcNintendoDirPath());
    out_fs.SetValue(std::make_shared<IFileSystemInterface>(std::move(redir_fs)));
    if (out_fs.IsDomain()) {
        out_fs.ChangeObjectId(sd_fs.s.object_id);
    }

    return ResultSuccess();
}

Result FsMitmService::OpenSaveDataFileSystem(Out<std::shared_ptr<IFileSystemInterface>> out_fs, u8 space_id, FsSave save_struct) {
    bool should_redirect_saves = false;
    const bool has_redirect_save_flags = Utils::HasFlag(static_cast<u64>(this->program_id), "redirect_save");
    if (R_FAILED(Utils::GetSettingsItemBooleanValue("atmosphere", "fsmitm_redirect_saves_to_sd", &should_redirect_saves))) {
        return ResultAtmosphereMitmShouldForwardToSession;
    }

    /* For now, until we're sure this is robust, only intercept normal savedata , check if flag exist*/
    if (!has_redirect_save_flags || !should_redirect_saves || save_struct.saveDataType != FsSaveDataType_SaveData) {
        return ResultAtmosphereMitmShouldForwardToSession;
    }

    /* Verify we can open the save. */
    FsFileSystem save_fs;
    if (R_FAILED(fsOpenSaveDataFileSystemFwd(this->forward_service.get(), &save_fs, space_id, &save_struct))) {
        return ResultAtmosphereMitmShouldForwardToSession;
    }
    std::unique_ptr<IFileSystem> save_ifs = std::make_unique<ProxyFileSystem>(save_fs);

    {
        /* Mount the SD card using fs.mitm's session. */
        FsFileSystem sd_fs;
        R_TRY(fsMountSdcard(&sd_fs));
        std::shared_ptr<IFileSystem> sd_ifs = std::make_shared<ProxyFileSystem>(sd_fs);


        /* Verify that we can open the save directory, and that it exists. */
        const u64 target_tid = save_struct.titleID == 0 ? static_cast<u64>(this->program_id) : save_struct.titleID;
        FsPath save_dir_path;
        R_TRY(FsSaveUtils::GetSaveDataDirectoryPath(save_dir_path, space_id, save_struct.saveDataType, target_tid, save_struct.userID, save_struct.saveID));

        /* Check if this is the first time we're making the save. */
        bool is_new_save = false;
        {
            DirectoryEntryType ent;
            if (sd_ifs->GetEntryType(&ent, save_dir_path) == ResultFsPathNotFound) {
                is_new_save = true;
            }
        }

        /* Ensure the directory exists. */
        R_TRY(FsDirUtils::EnsureDirectoryExists(sd_ifs.get(), save_dir_path));

        std::shared_ptr<DirectorySaveDataFileSystem> dirsave_ifs = std::make_shared<DirectorySaveDataFileSystem>(new SubDirectoryFileSystem(sd_ifs, save_dir_path.str), std::move(save_ifs));

        /* If it's the first time we're making the save, copy existing savedata over. */
        if (is_new_save) {
            /* TODO: Check error? */
            dirsave_ifs->CopySaveFromProxy();
        }

        out_fs.SetValue(std::make_shared<IFileSystemInterface>(static_cast<std::shared_ptr<IFileSystem>>(dirsave_ifs)));
        if (out_fs.IsDomain()) {
            out_fs.ChangeObjectId(sd_fs.s.object_id);
        }

        return ResultSuccess();
    }
}

/* Gate access to the BIS partitions. */
Result FsMitmService::OpenBisStorage(Out<std::shared_ptr<IStorageInterface>> out_storage, u32 _bis_partition_id) {
    const FsBisStorageId bis_partition_id = static_cast<FsBisStorageId>(_bis_partition_id);

    /* Try to open a storage for the partition. */
    FsStorage bis_storage;
    R_TRY(fsOpenBisStorageFwd(this->forward_service.get(), &bis_storage, bis_partition_id));

    const bool is_sysmodule = ams::ncm::IsSystemProgramId(this->program_id);
    const bool has_bis_write_flag = Utils::HasFlag(static_cast<u64>(this->program_id), "bis_write");
    const bool has_cal0_read_flag = Utils::HasFlag(static_cast<u64>(this->program_id), "cal_read");

    /* Set output storage. */
    if (bis_partition_id == FsBisStorageId_Boot0) {
        out_storage.SetValue(std::make_shared<IStorageInterface>(new Boot0Storage(bis_storage, this->program_id)));
    } else if (bis_partition_id == FsBisStorageId_CalibrationBinary) {
        /* PRODINFO should *never* be writable. */
        if (is_sysmodule || has_cal0_read_flag) {
            out_storage.SetValue(std::make_shared<IStorageInterface>(new ReadOnlyStorageAdapter(new ProxyStorage(bis_storage))));
        } else {
            /* Do not allow non-sysmodules to read *or* write CAL0. */
            fsStorageClose(&bis_storage);
            return ResultFsPermissionDenied;
        }
    } else {
        if (is_sysmodule || has_bis_write_flag) {
            /* Sysmodules should still be allowed to read and write. */
            out_storage.SetValue(std::make_shared<IStorageInterface>(new ProxyStorage(bis_storage)));
        } else if (Utils::IsHblTid(static_cast<u64>(this->program_id)) &&
            ((FsBisStorageId_BootConfigAndPackage2NormalMain <= bis_partition_id && bis_partition_id <= FsBisStorageId_BootConfigAndPackage2RepairSub) ||
            bis_partition_id == FsBisStorageId_Boot1)) {
            /* Allow HBL to write to boot1 (safe firm) + package2. */
            /* This is needed to not break compatibility with ChoiDujourNX, which does not check for write access before beginning an update. */
            /* TODO: get fixed so that this can be turned off without causing bricks :/ */
            out_storage.SetValue(std::make_shared<IStorageInterface>(new ProxyStorage(bis_storage)));
        } else {
            /* Non-sysmodules should be allowed to read. */
            out_storage.SetValue(std::make_shared<IStorageInterface>(new ReadOnlyStorageAdapter(new ProxyStorage(bis_storage))));
        }
    }

    /* Copy domain id. */
    if (out_storage.IsDomain()) {
        out_storage.ChangeObjectId(bis_storage.s.object_id);
    }

    return ResultSuccess();
}

/* Add redirection for RomFS to the SD card. */
Result FsMitmService::OpenDataStorageByCurrentProcess(Out<std::shared_ptr<IStorageInterface>> out_storage) {
    if (!this->should_override_contents) {
        return ResultAtmosphereMitmShouldForwardToSession;
    }

    /* If we don't have anything to modify, there's no sense in maintaining a copy of the metadata tables. */
    if (!Utils::HasSdRomfsContent(static_cast<u64>(this->program_id))) {
        return ResultAtmosphereMitmShouldForwardToSession;
    }

    /* Try to get from the cache. */
    {
        std::shared_ptr<IStorageInterface> cached_storage = nullptr;
        bool has_cache = StorageCacheGetEntry(this->program_id, &cached_storage);

        if (has_cache) {
            if (out_storage.IsDomain()) {
                /* TODO: Don't leak object id? */
                FsStorage s = {0};
                R_TRY(fsOpenDataStorageByCurrentProcessFwd(this->forward_service.get(), &s));
                out_storage.ChangeObjectId(s.s.object_id);
            }
            out_storage.SetValue(std::move(cached_storage));
            return ResultSuccess();
        }
    }

    /* Try to open process romfs. */
    FsStorage data_storage;
    R_TRY(fsOpenDataStorageByCurrentProcessFwd(this->forward_service.get(), &data_storage));

    /* Make new layered romfs, cacheing to storage. */
    {
        std::shared_ptr<IStorageInterface> storage_to_cache = nullptr;
        /* TODO: Is there a sensible path that ends in ".romfs" we can use?" */
        FsFile data_file;
        if (R_SUCCEEDED(Utils::OpenSdFileForAtmosphere(static_cast<u64>(this->program_id), "romfs.bin", FS_OPEN_READ, &data_file))) {
            storage_to_cache = std::make_shared<IStorageInterface>(new LayeredRomFS(std::make_shared<ReadOnlyStorageAdapter>(new ProxyStorage(data_storage)), std::make_shared<ReadOnlyStorageAdapter>(new FileStorage(new ProxyFile(data_file))), static_cast<u64>(this->program_id)));
        } else {
            storage_to_cache = std::make_shared<IStorageInterface>(new LayeredRomFS(std::make_shared<ReadOnlyStorageAdapter>(new ProxyStorage(data_storage)), nullptr, static_cast<u64>(this->program_id)));
        }

        StorageCacheSetEntry(this->program_id, &storage_to_cache);

        out_storage.SetValue(std::move(storage_to_cache));
        if (out_storage.IsDomain()) {
            out_storage.ChangeObjectId(data_storage.s.object_id);
        }
    }

    return ResultSuccess();
}

/* Add redirection for System Data Archives to the SD card. */
Result FsMitmService::OpenDataStorageByDataId(Out<std::shared_ptr<IStorageInterface>> out_storage, u64 data_id, u8 sid) {
    if (!this->should_override_contents) {
        return ResultAtmosphereMitmShouldForwardToSession;
    }

    /* If we don't have anything to modify, there's no sense in maintaining a copy of the metadata tables. */
    if (!Utils::HasSdRomfsContent(data_id)) {
        return ResultAtmosphereMitmShouldForwardToSession;
    }

    FsStorageId storage_id = static_cast<FsStorageId>(sid);

    /* Try to get from the cache. */
    {
        std::shared_ptr<IStorageInterface> cached_storage = nullptr;
        bool has_cache = StorageCacheGetEntry(ams::ncm::ProgramId{data_id}, &cached_storage);

        if (has_cache) {
            if (out_storage.IsDomain()) {
                /* TODO: Don't leak object id? */
                FsStorage s = {0};
                R_TRY(fsOpenDataStorageByDataIdFwd(this->forward_service.get(), storage_id, data_id, &s));
                out_storage.ChangeObjectId(s.s.object_id);
            }
            out_storage.SetValue(std::move(cached_storage));
            return ResultSuccess();
        }
    }

    /* Try to open data storage. */
    FsStorage data_storage;
    R_TRY(fsOpenDataStorageByDataIdFwd(this->forward_service.get(), storage_id, data_id, &data_storage));

    /* Make new layered romfs, cacheing to storage. */
    {
        std::shared_ptr<IStorageInterface> storage_to_cache = nullptr;
        /* TODO: Is there a sensible path that ends in ".romfs" we can use?" */
        FsFile data_file;
        if (R_SUCCEEDED(Utils::OpenSdFileForAtmosphere(data_id, "romfs.bin", FS_OPEN_READ, &data_file))) {
            storage_to_cache = std::make_shared<IStorageInterface>(new LayeredRomFS(std::make_shared<ReadOnlyStorageAdapter>(new ProxyStorage(data_storage)), std::make_shared<ReadOnlyStorageAdapter>(new FileStorage(new ProxyFile(data_file))), data_id));
        } else {
            storage_to_cache = std::make_shared<IStorageInterface>(new LayeredRomFS(std::make_shared<ReadOnlyStorageAdapter>(new ProxyStorage(data_storage)), nullptr, data_id));
        }

        StorageCacheSetEntry(ams::ncm::ProgramId{data_id}, &storage_to_cache);

        out_storage.SetValue(std::move(storage_to_cache));
        if (out_storage.IsDomain()) {
            out_storage.ChangeObjectId(data_storage.s.object_id);
        }
    }

    return ResultSuccess();
}
