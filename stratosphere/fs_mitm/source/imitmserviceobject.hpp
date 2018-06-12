#pragma once
#include <switch.h>

#include <stratosphere.hpp>

class IMitMServiceObject : public IServiceObject {
    protected:
        Service *forward_service;
    public:
        IMitMServiceObject(Service *s) : forward_service(s) {
            
        }
        
        virtual void clone_to(void *o) = 0;
    protected:
        virtual ~IMitMServiceObject() { }
        virtual Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) = 0;
        virtual void postprocess(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) = 0;
        virtual Result handle_deferred() = 0;
};
