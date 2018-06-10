#pragma once
#include <switch.h>
#include <stratosphere/iserviceobject.hpp>

class FsMitMService : IServiceObject {        
    public:
        virtual Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        virtual Result handle_deferred();
};