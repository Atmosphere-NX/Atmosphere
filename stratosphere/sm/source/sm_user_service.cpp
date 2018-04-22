#include <switch.h>
#include "sm_user_service.hpp"

Result UserService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    Result rc = 0xF601;
        
    switch ((UserServiceCmd)cmd_id) {
        case User_Cmd_Initialize:
            rc = WrapIpcCommandImpl<&UserService::initialize>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case User_Cmd_GetService:
            rc = WrapIpcCommandImpl<&UserService::get_service>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case User_Cmd_RegisterService:
            rc = WrapIpcCommandImpl<&UserService::register_service>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case User_Cmd_UnregisterService:
            rc = WrapIpcCommandImpl<&UserService::unregister_service>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        default:
            break;
    }
    return rc;
}


std::tuple<Result> UserService::initialize(PidDescriptor pid) {
    this->pid = pid.pid;
    return std::make_tuple(0);
}

std::tuple<Result, MovedHandle> UserService::get_service(u64 service) {
    /* TODO */
    return std::make_tuple(0xF601, MovedHandle{0});
}

std::tuple<Result, MovedHandle> UserService::register_service(u64 service, u8 is_light, u32 max_sessions) {
    /* TODO */
    return std::make_tuple(0xF601, MovedHandle{0});
}

std::tuple<Result> UserService::unregister_service(u64 service) {
    /* TODO */
    return std::make_tuple(0xF601);
}
