#pragma once
#include <switch.h>
#include <stratosphere/iserviceobject.hpp>
#include "imitmserviceobject.hpp"
#include "fs_istorage.hpp"


class GenericMitMService : public IMitMServiceObject {      
    public:
        GenericMitMService(Service *s) : IMitMServiceObject(s) {
            /* ... */
        }
        
        GenericMitMService *clone() override {
            auto new_srv = new GenericMitMService((Service *)&this->forward_service);
            this->clone_to(new_srv);
            return new_srv;
        }
        
        void clone_to(void *o) override {
            /* ... */
        }
        
        virtual Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        virtual void postprocess(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        virtual Result handle_deferred();
};