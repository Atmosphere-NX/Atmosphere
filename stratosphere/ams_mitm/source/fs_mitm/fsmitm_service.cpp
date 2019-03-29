/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include "fsmitm_romstorage.hpp"
#include "fsmitm_layeredrom.hpp"

#include "fs_subdirectory_filesystem.hpp"
#include "fs_directory_savedata_filesystem.hpp"

#include "../debug.hpp"

static HosMutex g_StorageCacheLock;
static std::unordered_map<u64, std::weak_ptr<IStorageInterface>> g_StorageCache;

static bool StorageCacheGetEntry(u64 title_id, std::shared_ptr<IStorageInterface> *out) {
    std::scoped_lock<HosMutex> lock(g_StorageCacheLock);
    if (g_StorageCache.find(title_id) == g_StorageCache.end()) {
        return false;
    }
    
    auto intf = g_StorageCache[title_id].lock();
    if (intf != nullptr) {
        *out = intf;
        return true;
    }
    return false;
}

static void StorageCacheSetEntry(u64 title_id, std::shared_ptr<IStorageInterface> *ptr) {
    std::scoped_lock<HosMutex> lock(g_StorageCacheLock);
    
    /* Ensure we always use the cached copy if present. */
    if (g_StorageCache.find(title_id) != g_StorageCache.end()) {
        auto intf = g_StorageCache[title_id].lock();
        if (intf != nullptr) {
            *ptr = intf;
        }
    }
    
    g_StorageCache[title_id] = *ptr;
}

void FsMitmService::PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx) {
    auto this_ptr = static_cast<FsMitmService *>(obj);
    switch ((FspSrvCmd)ctx->cmd_id) {
        case FspSrvCmd_SetCurrentProcess:
            if (R_SUCCEEDED(ctx->rc)) {
                this_ptr->has_initialized = true;
                this_ptr->process_id = ctx->request.Pid;
                this_ptr->title_id = this_ptr->process_id;
                if (R_FAILED(MitmQueryUtils::GetAssociatedTidForPid(this_ptr->process_id, &this_ptr->title_id))) {
                    /* Log here, if desired. */
                }
                break;
            }
            break;
        default:
            break;
    }
}

Result FsMitmService::OpenHblWebContentFileSystem(Out<std::shared_ptr<IFileSystemInterface>> &out_fs) {
    std::shared_ptr<IFileSystemInterface> fs = nullptr;
    u32 out_domain_id = 0;
    Result rc = ResultSuccess;
    
    ON_SCOPE_EXIT {
        if (R_SUCCEEDED(rc)) {
            out_fs.SetValue(std::move(fs));
            if (out_fs.IsDomain()) {
                out_fs.ChangeObjectId(out_domain_id);
            }
        }
    };
    
    /* Mount the SD card using fs.mitm's session. */
    FsFileSystem sd_fs;
    rc = fsMountSdcard(&sd_fs);
    if (R_SUCCEEDED(rc)) {
        fs = std::make_shared<IFileSystemInterface>(std::make_unique<SubDirectoryFileSystem>(std::make_shared<ProxyFileSystem>(sd_fs), AtmosphereHblWebContentDir));
        if (out_fs.IsDomain()) {
            out_domain_id = sd_fs.s.object_id;
        }
    }

    return rc;
}

Result FsMitmService::OpenFileSystemWithPatch(Out<std::shared_ptr<IFileSystemInterface>> out_fs, u64 title_id, u32 filesystem_type) {
    /* Check for eligibility. */
    {
        FsDir d;
        if (!Utils::IsWebAppletTid(this->title_id) || filesystem_type != FsFileSystemType_ContentManual || !Utils::IsHblTid(title_id) ||
            R_FAILED(Utils::OpenSdDir(AtmosphereHblWebContentDir, &d))) {
            return ResultAtmosphereMitmShouldForwardToSession;
        }
        fsDirClose(&d);
    }

    /* If there's an existing filesystem, don't override. */
    /* TODO: Multiplex, overriding existing content with HBL content. */
    {
        FsFileSystem fs;
        if (R_SUCCEEDED(fsOpenFileSystemWithPatchFwd(this->forward_service.get(), &fs, title_id, static_cast<FsFileSystemType>(filesystem_type)))) {
            fsFsClose(&fs);
            return ResultAtmosphereMitmShouldForwardToSession;
        }
    }
    
    return this->OpenHblWebContentFileSystem(out_fs);
}

Result FsMitmService::OpenFileSystemWithId(Out<std::shared_ptr<IFileSystemInterface>> out_fs, InPointer<char> path, u64 title_id, u32 filesystem_type) {
    /* Check for eligibility. */
    {
        FsDir d;
        if (!Utils::IsWebAppletTid(this->title_id) || filesystem_type != FsFileSystemType_ContentManual || !Utils::IsHblTid(title_id) || 
            R_FAILED(Utils::OpenSdDir(AtmosphereHblWebContentDir, &d))) {
            return ResultAtmosphereMitmShouldForwardToSession;
        }
        fsDirClose(&d);
    }

    /* If there's an existing filesystem, don't override. */
    /* TODO: Multiplex, overriding existing content with HBL content. */
    {
        FsFileSystem fs;
        if (R_SUCCEEDED(fsOpenFileSystemWithIdFwd(this->forward_service.get(), &fs, title_id, static_cast<FsFileSystemType>(filesystem_type), path.pointer))) {
            fsFsClose(&fs);
            return ResultAtmosphereMitmShouldForwardToSession;
        }
    }

    return this->OpenHblWebContentFileSystem(out_fs);
}

/* Gate access to the BIS partitions. */
Result FsMitmService::OpenBisStorage(Out<std::shared_ptr<IStorageInterface>> out_storage, u32 bis_partition_id) {
    std::shared_ptr<IStorageInterface> storage = nullptr;
    u32 out_domain_id = 0;
    Result rc = ResultSuccess;
    
    ON_SCOPE_EXIT {
        if (R_SUCCEEDED(rc)) {
            out_storage.SetValue(std::move(storage));
            if (out_storage.IsDomain()) {
                out_storage.ChangeObjectId(out_domain_id);
            }
        }
    };
    
    {
        FsStorage bis_storage;
        rc = fsOpenBisStorageFwd(this->forward_service.get(), &bis_storage, bis_partition_id);
        if (R_SUCCEEDED(rc)) {
            const bool is_sysmodule = TitleIdIsSystem(this->title_id);
            const bool has_bis_write_flag = Utils::HasFlag(this->title_id, "bis_write");
            const bool has_cal0_read_flag = Utils::HasFlag(this->title_id, "cal_read");
            if (bis_partition_id == BisStorageId_Boot0) {
                storage = std::make_shared<IStorageInterface>(new Boot0Storage(bis_storage, this->title_id));
            } else if (bis_partition_id == BisStorageId_Prodinfo) {
                /* PRODINFO should *never* be writable. */
                if (is_sysmodule || has_cal0_read_flag) {
                    storage = std::make_shared<IStorageInterface>(new ROProxyStorage(bis_storage));
                } else {
                    /* Do not allow non-sysmodules to read *or* write CAL0. */
                    fsStorageClose(&bis_storage);
                    return ResultFsPermissionDenied;
                }
            } else {
                if (is_sysmodule || has_bis_write_flag) {
                    /* Sysmodules should still be allowed to read and write. */
                    storage = std::make_shared<IStorageInterface>(new ProxyStorage(bis_storage));
                } else if (Utils::IsHblTid(this->title_id) && 
                    ((BisStorageId_BcPkg2_1 <= bis_partition_id && bis_partition_id <= BisStorageId_BcPkg2_6) || bis_partition_id == BisStorageId_Boot1)) {
                    /* Allow HBL to write to boot1 (safe firm) + package2. */
                    /* This is needed to not break compatibility with ChoiDujourNX, which does not check for write access before beginning an update. */
                    /* TODO: get fixed so that this can be turned off without causing bricks :/ */
                    storage = std::make_shared<IStorageInterface>(new ProxyStorage(bis_storage));
                } else {
                    /* Non-sysmodules should be allowed to read. */
                    storage = std::make_shared<IStorageInterface>(new ROProxyStorage(bis_storage));
                }
            }
            if (out_storage.IsDomain()) {
                out_domain_id = bis_storage.s.object_id;
            }
        }
    }
    
    return rc;
}

/* Add redirection for RomFS to the SD card. */
Result FsMitmService::OpenDataStorageByCurrentProcess(Out<std::shared_ptr<IStorageInterface>> out_storage) {
    std::shared_ptr<IStorageInterface> storage = nullptr;
    u32 out_domain_id = 0;
    Result rc = ResultSuccess;
    
    if (!this->should_override_contents) {
        return ResultAtmosphereMitmShouldForwardToSession;
    }
    
    bool has_cache = StorageCacheGetEntry(this->title_id, &storage);
    
    ON_SCOPE_EXIT {
        if (R_SUCCEEDED(rc)) {
            if (!has_cache) {
                StorageCacheSetEntry(this->title_id, &storage);
            }
            
            out_storage.SetValue(std::move(storage));
            if (out_storage.IsDomain()) {
                out_storage.ChangeObjectId(out_domain_id);
            }
        }
    };
    
    
    if (has_cache) {
        if (out_storage.IsDomain()) {
            FsStorage s = {0};
            rc = fsOpenDataStorageByCurrentProcessFwd(this->forward_service.get(), &s);
            if (R_SUCCEEDED(rc)) {
                out_domain_id = s.s.object_id;
            }
        } else {
            rc = ResultSuccess;
        }
        if (R_FAILED(rc)) {
            storage.reset();
        }
    } else {
        FsStorage data_storage;
        FsFile data_file;
        
        rc = fsOpenDataStorageByCurrentProcessFwd(this->forward_service.get(), &data_storage);

        Log(armGetTls(), 0x100);
        if (R_SUCCEEDED(rc)) {
            if (Utils::HasSdRomfsContent(this->title_id)) {
                /* TODO: Is there a sensible path that ends in ".romfs" we can use?" */
                if (R_SUCCEEDED(Utils::OpenSdFileForAtmosphere(this->title_id, "romfs.bin", FS_OPEN_READ, &data_file))) {
                    storage = std::make_shared<IStorageInterface>(new LayeredRomFS(std::make_shared<RomInterfaceStorage>(data_storage), std::make_shared<RomFileStorage>(data_file), this->title_id));
                } else {
                    storage = std::make_shared<IStorageInterface>(new LayeredRomFS(std::make_shared<RomInterfaceStorage>(data_storage), nullptr, this->title_id));
                }
                if (out_storage.IsDomain()) {
                    out_domain_id = data_storage.s.object_id;
                }
            } else {
                /* If we don't have anything to modify, there's no sense in maintaining a copy of the metadata tables. */
                fsStorageClose(&data_storage);
                rc = ResultAtmosphereMitmShouldForwardToSession;
            }
        }
    }
    
    return rc;
}

/* Add redirection for System Data Archives to the SD card. */
Result FsMitmService::OpenDataStorageByDataId(Out<std::shared_ptr<IStorageInterface>> out_storage, u64 data_id, u8 sid) {
    FsStorageId storage_id = (FsStorageId)sid;
    FsStorage data_storage;
    FsFile data_file;
    
    if (!this->should_override_contents) {
        return ResultAtmosphereMitmShouldForwardToSession;
    }
        
    std::shared_ptr<IStorageInterface> storage = nullptr;
    u32 out_domain_id = 0;
    Result rc = ResultSuccess;
    
    bool has_cache = StorageCacheGetEntry(data_id, &storage);
    
    ON_SCOPE_EXIT {
        if (R_SUCCEEDED(rc)) {
            if (!has_cache) {
                StorageCacheSetEntry(data_id, &storage);
            }
            
            out_storage.SetValue(std::move(storage));
            if (out_storage.IsDomain()) {
                out_storage.ChangeObjectId(out_domain_id);
            }
        }
    };
    
    if (has_cache) {
        if (out_storage.IsDomain()) {
            FsStorage s = {0};
            rc = fsOpenDataStorageByDataIdFwd(this->forward_service.get(), storage_id, data_id, &s);
            if (R_SUCCEEDED(rc)) {
                out_domain_id = s.s.object_id;
            }
        } else {
            rc = ResultSuccess;
        }
        if (R_FAILED(rc)) {
            storage.reset();
        }
    } else {
        rc = fsOpenDataStorageByDataIdFwd(this->forward_service.get(), storage_id, data_id, &data_storage);

        if (R_SUCCEEDED(rc)) {
            if (Utils::HasSdRomfsContent(data_id)) {
                /* TODO: Is there a sensible path that ends in ".romfs" we can use?" */
                if (R_SUCCEEDED(Utils::OpenSdFileForAtmosphere(data_id, "romfs.bin", FS_OPEN_READ, &data_file))) {
                    storage = std::make_shared<IStorageInterface>(new LayeredRomFS(std::make_shared<RomInterfaceStorage>(data_storage), std::make_shared<RomFileStorage>(data_file), data_id));
                } else {
                    storage = std::make_shared<IStorageInterface>(new LayeredRomFS(std::make_shared<RomInterfaceStorage>(data_storage), nullptr, data_id));
                }
                if (out_storage.IsDomain()) {
                    out_domain_id = data_storage.s.object_id;
                }
            } else {
                /* If we don't have anything to modify, there's no sense in maintaining a copy of the metadata tables. */
                fsStorageClose(&data_storage);
                rc = ResultAtmosphereMitmShouldForwardToSession;
            }
        }
    }
    
    return rc;
}