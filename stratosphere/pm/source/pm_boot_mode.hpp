#pragma once
#include <switch.h>
#include <stratosphere/iserviceobject.hpp>

enum BootModeCmd {
    BootMode_Cmd_GetBootMode = 0,
    BootMode_Cmd_SetMaintenanceBoot = 1
};

class BootModeService : IServiceObject {
    public:
        virtual Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        virtual Result handle_deferred();
        
    private:
        /* Actual commands. */
        std::tuple<Result, bool> get_boot_mode();
        std::tuple<Result> set_maintenance_boot();
};