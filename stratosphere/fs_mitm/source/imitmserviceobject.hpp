#pragma once
#include <switch.h>

#include <stratosphere.hpp>

class IMitMServiceObject : public IServiceObject {
    protected:
        Service *forward_service;
    public:
        IMitMServiceObject(Service *s) : forward_service(s) {
            
        }
    protected:
        virtual ~IMitMServiceObject() { }
        virtual Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) = 0;
        virtual Result postprocess(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) = 0;
        virtual Result handle_deferred() = 0;
};
