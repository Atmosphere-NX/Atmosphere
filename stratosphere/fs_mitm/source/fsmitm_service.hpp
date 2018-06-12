#pragma once
#include <switch.h>
#include <stratosphere/iserviceobject.hpp>
#include "imitmserviceobject.hpp"
#include "fs_istorage.hpp"

enum FspSrvCmd {
    FspSrv_Cmd_SetCurrentProcess = 1,
    FspSrv_Cmd_OpenDataStorageByDataId = 202,
};

class FsMitMService : public IMitMServiceObject {      
    private:
        bool has_initialized;
        u64 process_id;
        u64 title_id;
    public:
        FsMitMService(Service *s) : IMitMServiceObject(s), has_initialized(false), process_id(0), title_id(0) {
            /* ... */
        }
        
        FsMitMService *clone() override {
            auto new_srv = new FsMitMService((Service *)&this->forward_service);
            this->clone_to(new_srv);
            return new_srv;
        }
        
        void clone_to(void *o) override {
            FsMitMService *other = (FsMitMService *)o;
            other->has_initialized = has_initialized;
            other->process_id = process_id;
            other->title_id = title_id;
        }
        
        virtual Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        virtual void postprocess(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        virtual Result handle_deferred();
    
    protected:
        /* Overridden commands. */
        std::tuple<Result, OutSession<IStorageInterface>> open_data_storage_by_data_id(u64 storage_id, u64 data_id);
};