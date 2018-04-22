#include <switch.h>
#include "sm_manager_service.hpp"
#include "sm_registration.hpp"

Result ManagerService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    Result rc = 0xF601;
        
    switch ((ManagerServiceCmd)cmd_id) {
        case Manager_Cmd_RegisterProcess:
            rc = WrapIpcCommandImpl<&ManagerService::register_process>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Manager_Cmd_UnregisterProcess:
            rc = WrapIpcCommandImpl<&ManagerService::unregister_process>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        default:
            break;
    }
    return rc;
}

Result ManagerService::handle_deferred() {
    /* This service is never deferrable. */
    return 0;
}


std::tuple<Result> ManagerService::register_process(u64 pid, InBuffer<u8> acid_sac, InBuffer<u8> aci0_sac) {
    return std::make_tuple(Registration::RegisterProcess(pid, acid_sac.buffer, acid_sac.num_elements, aci0_sac.buffer, aci0_sac.num_elements));
}

std::tuple<Result> ManagerService::unregister_process(u64 pid) {
    return std::make_tuple(Registration::UnregisterProcess(pid));
}