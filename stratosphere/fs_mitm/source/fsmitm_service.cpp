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

#include "fsmitm_worker.hpp"
#include "fsmitm_utils.hpp"
#include "fsmitm_romstorage.hpp"
#include "fsmitm_layeredrom.hpp"

#include "mitm_query_service.hpp"
#include "debug.hpp"

Result FsMitMService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    Result rc = 0xF601;
    if (this->has_initialized) {
        switch (static_cast<FspSrvCmd>(cmd_id)) {
            case FspSrvCmd::OpenDataStorageByCurrentProcess:
                rc = WrapIpcCommandImpl<&FsMitMService::open_data_storage_by_current_process>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case FspSrvCmd::OpenDataStorageByDataId:
                rc = WrapIpcCommandImpl<&FsMitMService::open_data_storage_by_data_id>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            default:
                break;
        }
    } else {
        if (static_cast<FspSrvCmd>(cmd_id) == FspSrvCmd::SetCurrentProcess) {
            if (r.HasPid) {
                this->init_pid = r.Pid;
            }
        }
    }
    return rc;
}

void FsMitMService::postprocess(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    struct {
        u64 magic;
        u64 result;
    } *resp = (decltype(resp))r.Raw;
    
    u64 *tls = (u64 *)armGetTls();
    std::array<u64, 0x100/sizeof(u64)> backup_tls;
    std::copy(tls, tls + backup_tls.size(), backup_tls.begin());
    
    Result rc = (Result)resp->result;
    switch (static_cast<FspSrvCmd>(cmd_id)) {
        case FspSrvCmd::SetCurrentProcess:
            if (R_SUCCEEDED(rc)) {
                this->has_initialized = true;
            }
            this->process_id = this->init_pid;
            this->title_id = this->process_id;
            if (R_FAILED(MitMQueryUtils::get_associated_tid_for_pid(this->process_id, &this->title_id))) {
                /* Log here, if desired. */
            }
            std::copy(backup_tls.begin(), backup_tls.end(), tls);
            break;
        default:
            break;
    }
    resp->result = rc;
}

Result FsMitMService::handle_deferred() {
    /* This service is never deferrable. */
    return 0;
}

/* Add redirection for RomFS to the SD card. */
std::tuple<Result, OutSession<IStorageInterface>> FsMitMService::open_data_storage_by_current_process() {
    IPCSession<IStorageInterface> *out_session = NULL;
    std::shared_ptr<IStorageInterface> out_storage = nullptr;
    u32 out_domain_id = 0;
    Result rc;
    if (this->romfs_storage != nullptr) {
        if (this->get_owner() != NULL) {
            rc = fsOpenDataStorageByCurrentProcessFromDomainFwd(this->forward_service, &out_domain_id);
        } else {
            rc = 0;
        }
        if (R_SUCCEEDED(rc)) {
            out_storage = this->romfs_storage;
            out_session = new IPCSession<IStorageInterface>(out_storage);
        }
    } else {
        FsStorage data_storage;
        FsFile data_file;

        if (this->get_owner() == NULL) {
            rc = fsOpenDataStorageByCurrentProcessFwd(this->forward_service, &data_storage);
        } else {
            rc = fsOpenDataStorageByCurrentProcessFromDomainFwd(this->forward_service, &out_domain_id);
            if (R_SUCCEEDED(rc)) {
                rc = ipcCopyFromDomain(this->forward_service->handle, out_domain_id, &data_storage.s);
            }
        }
        Log(armGetTls(), 0x100);
        if (R_SUCCEEDED(rc)) {
            /* TODO: Is there a sensible path that ends in ".romfs" we can use?" */
            if (R_SUCCEEDED(Utils::OpenSdFileForAtmosphere(this->title_id, "romfs.bin", FS_OPEN_READ, &data_file))) {
                out_storage = std::make_shared<IStorageInterface>(new LayeredRomFS(std::make_shared<RomInterfaceStorage>(data_storage), std::make_shared<RomFileStorage>(data_file), this->title_id));
            } else {
                out_storage = std::make_shared<IStorageInterface>(new LayeredRomFS(std::make_shared<RomInterfaceStorage>(data_storage), nullptr, this->title_id));
            }
            this->romfs_storage = out_storage;
            out_session = new IPCSession<IStorageInterface>(out_storage);
            if (this->get_owner() == NULL) {
                FsMitMWorker::AddWaitable(out_session);
            }
        }
    }
    
    OutSession out_s = OutSession(out_session);
    out_s.domain_id = out_domain_id;
    return {rc, out_s};
}

/* Add redirection for System Data Archives to the SD card. */
std::tuple<Result, OutSession<IStorageInterface>> FsMitMService::open_data_storage_by_data_id(u64 sid, u64 data_id) {
    FsStorageId storage_id = (FsStorageId)sid;
    IPCSession<IStorageInterface> *out_session = NULL;
    FsStorage data_storage;
    FsFile data_file;
    u32 out_domain_id = 0;
    Result rc;
    if (this->get_owner() == NULL) {
        rc = fsOpenDataStorageByDataId(this->forward_service, storage_id, data_id, &data_storage);
    } else {
        rc = fsOpenDataStorageByDataIdFromDomain(this->forward_service, storage_id, data_id, &out_domain_id);
        if (R_SUCCEEDED(rc)) {
            rc = ipcCopyFromDomain(this->forward_service->handle, out_domain_id, &data_storage.s);
        }
    }
    if (R_SUCCEEDED(rc)) {
        /* TODO: Is there a sensible path that ends in ".romfs" we can use?" */
        if (R_SUCCEEDED(Utils::OpenSdFileForAtmosphere(data_id, "romfs.bin", FS_OPEN_READ, &data_file))) {
            out_session = new IPCSession<IStorageInterface>(std::make_shared<IStorageInterface>(new LayeredRomFS(std::make_shared<RomInterfaceStorage>(data_storage), std::make_shared<RomFileStorage>(data_file), data_id)));
        } else {
            out_session = new IPCSession<IStorageInterface>(std::make_shared<IStorageInterface>(new LayeredRomFS(std::make_shared<RomInterfaceStorage>(data_storage), nullptr, data_id)));
        }
        if (this->get_owner() == NULL) {
            FsMitMWorker::AddWaitable(out_session);
        }
    }
    
    OutSession out_s = OutSession(out_session);
    out_s.domain_id = out_domain_id;
    return {rc, out_s};
}