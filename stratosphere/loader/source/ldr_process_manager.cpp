#include <switch.h>
#include "ldr_process_manager.hpp"
#include "ldr_registration.hpp"
#include "ldr_launch_queue.hpp"

Result ProcessManagerService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    
    Result rc = 0xF601;
        
    switch ((ProcessManagerServiceCmd)cmd_id) {
        case Pm_Cmd_CreateProcess:
            rc = WrapIpcCommandImpl<&ProcessManagerService::create_process>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Pm_Cmd_GetProgramInfo:
            rc = WrapIpcCommandImpl<&ProcessManagerService::get_program_info>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Pm_Cmd_RegisterTitle:            
            rc = WrapIpcCommandImpl<&ProcessManagerService::register_title>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Pm_Cmd_UnregisterTitle:
            rc = WrapIpcCommandImpl<&ProcessManagerService::unregister_title>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        default:
            break;
    }
    return rc;
}

std::tuple<Result> ProcessManagerService::create_process() {
    /* TODO */
    return std::make_tuple(0xF601);
}

std::tuple<Result> ProcessManagerService::get_program_info(Registration::TidSid tid_sid, OutPointerWithServerSize<ProcessManagerService::ProgramInfo, 0x1> out_program_info) {
    /* Zero output. */
    std::fill(out_program_info.pointer, out_program_info.pointer + out_program_info.num_elements, (const ProcessManagerService::ProgramInfo){0});
    
    return std::make_tuple(0xA09);
}

std::tuple<Result, u64> ProcessManagerService::register_title(Registration::TidSid tid_sid) {
    u64 out_index = 0;
    if (Registration::register_tid_sid(&tid_sid, &out_index)) {
        return std::make_tuple(0, out_index);
    } else {
        return std::make_tuple(0xE09, out_index);
    }
}

std::tuple<Result> ProcessManagerService::unregister_title(u64 index) {
    if (Registration::unregister_index(index)) {
        return std::make_tuple(0);
    } else {
        return std::make_tuple(0x1009);
    }
}