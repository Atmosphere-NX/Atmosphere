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
 
#include <switch.h>
#include "fsmitm_service.hpp"
#include "fs_shim.h"

#include "fsmitm_utils.hpp"
#include "fsmitm_romstorage.hpp"
#include "fsmitm_layeredrom.hpp"

#include "debug.hpp"

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

/* Add redirection for RomFS to the SD card. */
Result FsMitmService::OpenDataStorageByCurrentProcess(Out<std::shared_ptr<IStorageInterface>> out_storage) {
    std::shared_ptr<IStorageInterface> storage = nullptr;
    u32 out_domain_id = 0;
    Result rc = 0;
    
    ON_SCOPE_EXIT {
        if (R_SUCCEEDED(rc)) {
            out_storage.SetValue(std::move(storage));
            if (out_storage.IsDomain()) {
                out_storage.ChangeObjectId(out_domain_id);
            }
        }
    };
        
    if (this->romfs_storage != nullptr) {
        if (out_storage.IsDomain()) {
            FsStorage s = {0};
            rc = fsOpenDataStorageByCurrentProcessFwd(this->forward_service.get(), &s);
            if (R_SUCCEEDED(rc)) {
                out_domain_id = s.s.object_id;
            }
        } else {
            rc = 0;
        }
        if (R_SUCCEEDED(rc)) {
            storage = this->romfs_storage;
        }
    } else {
        FsStorage data_storage;
        FsFile data_file;
        
        rc = fsOpenDataStorageByCurrentProcessFwd(this->forward_service.get(), &data_storage);

        Log(armGetTls(), 0x100);
        if (R_SUCCEEDED(rc)) {
            /* TODO: Is there a sensible path that ends in ".romfs" we can use?" */
            if (R_SUCCEEDED(Utils::OpenSdFileForAtmosphere(this->title_id, "romfs.bin", FS_OPEN_READ, &data_file))) {
                storage = std::make_shared<IStorageInterface>(new LayeredRomFS(std::make_shared<RomInterfaceStorage>(data_storage), std::make_shared<RomFileStorage>(data_file), this->title_id));
            } else {
                storage = std::make_shared<IStorageInterface>(new LayeredRomFS(std::make_shared<RomInterfaceStorage>(data_storage), nullptr, this->title_id));
            }
            this->romfs_storage = storage;
            if (out_storage.IsDomain()) {
                out_domain_id = data_storage.s.object_id;
            }
        }
    }
    
    return rc;
}

/* Add redirection for System Data Archives to the SD card. */
Result FsMitmService::OpenDataStorageByDataId(Out<std::shared_ptr<IStorageInterface>> out_storage, u64 sid, u64 data_id) {
    FsStorageId storage_id = (FsStorageId)sid;
    FsStorage data_storage;
    FsFile data_file;
    
    std::shared_ptr<IStorageInterface> storage = nullptr;
    u32 out_domain_id = 0;
    Result rc = 0;
    
    ON_SCOPE_EXIT {
        if (R_SUCCEEDED(rc)) {
            out_storage.SetValue(std::move(storage));
            if (out_storage.IsDomain()) {
                out_storage.ChangeObjectId(out_domain_id);
            }
        }
    };
    
    rc = fsOpenDataStorageByDataIdFwd(this->forward_service.get(), storage_id, data_id, &data_storage);

    if (R_SUCCEEDED(rc)) {
        /* TODO: Is there a sensible path that ends in ".romfs" we can use?" */
        if (R_SUCCEEDED(Utils::OpenSdFileForAtmosphere(data_id, "romfs.bin", FS_OPEN_READ, &data_file))) {
            storage = std::make_shared<IStorageInterface>(new LayeredRomFS(std::make_shared<RomInterfaceStorage>(data_storage), std::make_shared<RomFileStorage>(data_file), data_id));
        } else {
            storage = std::make_shared<IStorageInterface>(new LayeredRomFS(std::make_shared<RomInterfaceStorage>(data_storage), nullptr, data_id));
        }
        if (out_storage.IsDomain()) {
            out_domain_id = data_storage.s.object_id;
        }
    }
    
    return rc;
}