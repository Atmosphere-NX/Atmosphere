#include <switch.h>
#include <cstdio>
#include <algorithm>

#include "ldr_ro_service.hpp"
#include "ldr_registration.hpp"

Result RelocatableObjectsService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    Result rc = 0xF601;
        
    switch ((RoServiceCmd)cmd_id) {
        case Ro_Cmd_LoadNro:
            rc = WrapIpcCommandImpl<&RelocatableObjectsService::load_nro>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Ro_Cmd_UnloadNro:
            rc = WrapIpcCommandImpl<&RelocatableObjectsService::unload_nro>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Ro_Cmd_LoadNrr:
            rc = WrapIpcCommandImpl<&RelocatableObjectsService::load_nrr>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Ro_Cmd_UnloadNrr:
            rc = WrapIpcCommandImpl<&RelocatableObjectsService::unload_nrr>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Ro_Cmd_Initialize:
            rc = WrapIpcCommandImpl<&RelocatableObjectsService::initialize>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        default:
            break;
    }
    return rc;
}


std::tuple<Result, u64> load_nro(PidDescriptor pid, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size) {
    /* TODO */
    return std::make_tuple(0xF601, 0);
}

std::tuple<Result> unload_nro(PidDescriptor pid, u64 nro_address) {
    /* TODO */
    return std::make_tuple(0xF601);
}

std::tuple<Result> load_nrr(PidDescriptor pid, u64 nrr_address, u64 nrr_size) {
    /* TODO */
    return std::make_tuple(0xF601);
}

std::tuple<Result> unload_nrr(PidDescriptor pid, u64 nrr_address) {
    /* TODO */
    return std::make_tuple(0xF601);
}

std::tuple<Result> initialize(PidDescriptor pid, CopiedHandle process_h) {
    /* TODO */
    return std::make_tuple(0xF601);
}
