#include <switch.h>
#include "fsmitm_service.hpp"
#include "fs_shim.h"

#include "fsmitm_worker.hpp"
#include "fsmitm_utils.hpp"
#include "fsmitm_romstorage.hpp"

#include "debug.hpp"

Result FsMitMService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    Result rc = 0xF601;
    
    switch (cmd_id) {
        case FspSrv_Cmd_SetCurrentProcess:
            if (!this->has_initialized && r.HasPid) {
                this->process_id = r.Pid;
            }
            break;
        /*case FspSrv_Cmd_OpenDataStorageByDataId:
            rc = WrapIpcCommandImpl<&FsMitMService::open_data_storage_by_data_id>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break; */
    }
    return rc;
}

void FsMitMService::postprocess(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    struct {
        u64 magic;
        u64 result;
    } *resp = (decltype(resp))r.Raw;
    
    u64 *tls = (u64 *)armGetTls();
    u64 backup_tls[0x100/sizeof(u64)];
    for (unsigned int i = 0; i < sizeof(backup_tls)/sizeof(u64); i++) {
        backup_tls[i] = tls[i];
    }
    
    Result rc = (Result)resp->result;
    switch (cmd_id) {
        case FspSrv_Cmd_SetCurrentProcess:
            if (R_SUCCEEDED(rc)) {
                this->has_initialized = true;
                rc = pminfoGetTitleId(&this->title_id, this->process_id);
                if (R_FAILED(rc)) {
                    if (rc == 0x20F) {
                        this->title_id = this->process_id;
                        rc = 0x0;
                    } else {
                        fatalSimple(rc);
                    }
                }
            }
            Log(&this->process_id, 8);
            Log(&this->title_id, 8);
            for (unsigned int i = 0; i < sizeof(backup_tls)/sizeof(u64); i++) {
                tls[i] = backup_tls[i];
            }
            if (this->title_id >= 0x0100000000001000) {
                Reboot();
            }
            break;
    }
    resp->result = rc;
}

Result FsMitMService::handle_deferred() {
    /* This service is never deferrable. */
    return 0;
}

/* Add redirection for System Data Archives to the SD card. */
std::tuple<Result, OutSession<IStorageInterface>> FsMitMService::open_data_storage_by_data_id(u64 sid, u64 data_id) {
    FsStorageId storage_id = (FsStorageId)sid;
    IPCSession<IStorageInterface> *out_session = NULL;
    FsStorage data_storage;
    FsFile data_file;
    u32 out_domain_id;
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
        char path[FS_MAX_PATH] = {0};
        /* TODO: Is there a sensible path that ends in ".romfs" we can use?" */
        snprintf(path, sizeof(path), "/atmosphere/titles/%016lx/romfs.bin", data_id);
        if (R_SUCCEEDED(Utils::OpenSdFile(path, FS_OPEN_READ, &data_file))) {
            fsStorageClose(&data_storage);
            out_session = new IPCSession<IStorageInterface>(new IStorageInterface(new RomFileStorage(data_file)));
        } else {     
            out_session = new IPCSession<IStorageInterface>(new IStorageInterface(new RomInterfaceStorage(data_storage)));
        }
        if (this->get_owner() == NULL) {
            FsMitmWorker::AddWaitable(out_session);
        }
    }
    
    OutSession out_s = OutSession(out_session);
    out_s.domain_id = out_domain_id;
    return {rc, out_s};
}