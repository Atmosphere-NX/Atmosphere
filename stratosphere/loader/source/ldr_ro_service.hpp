#pragma once
#include <switch.h>

#include <stratosphere/iserviceobject.hpp>
#include "ldr_registration.hpp"

enum RoServiceCmd {
    Ro_Cmd_LoadNro = 0,
    Ro_Cmd_UnloadNro = 1,
    Ro_Cmd_LoadNrr = 2,
    Ro_Cmd_UnloadNrr = 3,
    Ro_Cmd_Initialize = 4,
};

class RelocatableObjectsService : IServiceObject {
    Handle process_handle;
    u64 process_id;
    bool has_initialized;
    public:
        RelocatableObjectsService() : process_handle(0), process_id(U64_MAX), has_initialized(false) { }
        ~RelocatableObjectsService() {
            Registration::CloseRoService(this, this->process_handle);
            if (this->has_initialized) {
                svcCloseHandle(this->process_handle);
            }
        }
        virtual Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        virtual Result handle_deferred() {
            /* This service will never defer. */
            return 0;
        }
        
    private:
        /* Actual commands. */
        std::tuple<Result, u64> load_nro(PidDescriptor pid_desc, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size);
        std::tuple<Result> unload_nro(PidDescriptor pid_desc, u64 nro_address);
        std::tuple<Result> load_nrr(PidDescriptor pid_desc, u64 nrr_address, u64 nrr_size);
        std::tuple<Result> unload_nrr(PidDescriptor pid_desc, u64 nrr_address);
        std::tuple<Result> initialize(PidDescriptor pid_desc, CopiedHandle process_h);
};