#pragma once
#include <switch.h>
#include <atomic>

#include <stratosphere.hpp>

#include "debug.hpp"

class IMitMServiceObject : public IServiceObject {
    protected:
        Service *forward_service;
        u64 process_id = 0;
        u64 title_id = 0;
    public:
        IMitMServiceObject(Service *s) : forward_service(s) {
            
        }
        
        static bool should_mitm(u64 pid, u64 tid) {
            return true;
        }
                
        virtual void clone_to(void *o) = 0;
    protected:
        virtual ~IMitMServiceObject() = default;
        virtual Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) = 0;
        virtual void postprocess(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) = 0;
        virtual Result handle_deferred() = 0;
};
