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

class RelocatableObjectsService final : public IServiceObject {
    Handle process_handle = 0;
    u64 process_id = U64_MAX;
    bool has_initialized = false;
    public:
        ~RelocatableObjectsService() {
            Registration::CloseRoService(this, this->process_handle);
            if (this->has_initialized) {
                svcCloseHandle(this->process_handle);
            }
        }
        Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) override;
        Result handle_deferred() override {
            /* This service will never defer. */
            return 0;
        }
        
        RelocatableObjectsService *clone() override {
            return new RelocatableObjectsService(*this);
        }
        
    private:
        /* Actual commands. */
        std::tuple<Result, u64> load_nro(PidDescriptor pid_desc, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size);
        std::tuple<Result> unload_nro(PidDescriptor pid_desc, u64 nro_address);
        std::tuple<Result> load_nrr(PidDescriptor pid_desc, u64 nrr_address, u64 nrr_size);
        std::tuple<Result> unload_nrr(PidDescriptor pid_desc, u64 nrr_address);
        std::tuple<Result> initialize(PidDescriptor pid_desc, CopiedHandle process_h);
};
