#include <switch.h>
#include "pm_boot_mode.hpp"

static bool g_is_maintenance_boot = false;

Result BootModeService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    Result rc = 0xF601;
    switch ((BootModeCmd)cmd_id) {
        case BootMode_Cmd_GetBootMode:
            rc = WrapIpcCommandImpl<&BootModeService::get_boot_mode>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case BootMode_Cmd_SetMaintenanceBoot:
            rc = WrapIpcCommandImpl<&BootModeService::set_maintenance_boot>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        default:
            break;
    }
    return rc;
}

Result BootModeService::handle_deferred() {
    /* This service is never deferrable. */
    return 0;
}

std::tuple<Result, bool> BootModeService::get_boot_mode() {
    return {0, g_is_maintenance_boot};
}

std::tuple<Result> BootModeService::set_maintenance_boot() {
    g_is_maintenance_boot = true;
    return {0};
}
