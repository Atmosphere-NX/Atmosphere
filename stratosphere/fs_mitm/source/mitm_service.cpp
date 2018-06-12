#include <switch.h>
#include "mitm_service.hpp"

#include "debug.hpp"

Result GenericMitMService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    return 0xF601;
}

void GenericMitMService::postprocess(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
}

Result GenericMitMService::handle_deferred() {
    /* This service is never deferrable. */
    return 0;
}