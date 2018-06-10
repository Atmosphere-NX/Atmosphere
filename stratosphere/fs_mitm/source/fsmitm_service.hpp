#pragma once
#include <switch.h>
#include <stratosphere/iserviceobject.hpp>
#include "imitmserviceobject.hpp"

enum FspSrvCmd {
    FspSrv_Cmd_SetCurrentProcess = 1,
};

class FsMitMService : public IMitMServiceObject {      
    private:
        bool has_initialized;
        u64 process_id;
        u64 title_id;
    public:
        virtual Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        virtual Result postprocess(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        virtual Result handle_deferred();
};