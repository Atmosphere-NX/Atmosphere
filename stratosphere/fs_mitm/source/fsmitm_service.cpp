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
        case FspSrv_Cmd_OpenDataStorageByDataId:
            rc = WrapIpcCommandImpl<&FsMitMService::open_data_storage_by_data_id>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
    }
    return rc;
}

Result FsMitMService::postprocess(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    struct {
        u64 magic;
        u64 result;
    } *resp = (decltype(resp))r.Raw;
    
    Result rc = (Result)resp->result;
    switch (cmd_id) {
        case FspSrv_Cmd_SetCurrentProcess:
            if (R_SUCCEEDED(rc)) {
                this->has_initialized = true;
                if (R_FAILED(pminfoInitialize()) || R_FAILED(pminfoGetTitleId(&this->title_id, this->process_id))) {
                    fatalSimple(0xCAFE << 8 | 0xFD);
                }
                pminfoExit();
            }
            break;
    }
    return rc;
}

Result FsMitMService::handle_deferred() {
    /* This service is never deferrable. */
    return 0;
}

/* Add redirection for System Data Archives to the SD card. */
std::tuple<Result, MovedHandle> FsMitMService::open_data_storage_by_data_id(FsStorageId storage_id, u64 data_id) {
    Handle out_h = 0;
    FsStorage data_storage;
    FsFile data_file;
    Result rc = fsOpenDataStorageByDataId(this->forward_service, storage_id, data_id, &data_storage);
    if (R_SUCCEEDED(rc)) {
        IPCSession<IStorageInterface> *out_session = NULL;
        char path[FS_MAX_PATH] = {0};
        /* TODO: Is there a sensible path that ends in ".romfs" we can use?" */
        snprintf(path, sizeof(path), "/atmosphere/titles/%016lx/romfs.bin", data_id);
        if (R_SUCCEEDED(Utils::OpenSdFile(path, FS_OPEN_READ, &data_file))) {
            fsStorageClose(&data_storage);
            out_session = new IPCSession<IStorageInterface>(new IStorageInterface(new RomFileStorage(data_file)));
        } else {
            
            out_session = new IPCSession<IStorageInterface>(new IStorageInterface(new RomInterfaceStorage(data_storage)));
        }
        FsMitmWorker::AddWaitable(out_session);
        out_h = out_session->get_client_handle();
    }
    return {rc, out_h};
}