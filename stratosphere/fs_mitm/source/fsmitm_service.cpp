#include <switch.h>
#include "fsmitm_service.hpp"

Result FsMitMService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    Result rc = 0xF601;
    switch (cmd_id) {
        case FspSrv_Cmd_SetCurrentProcess:
            if (!this->has_initialized && r.HasPid) {
                this->process_id = r.Pid;
            }
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