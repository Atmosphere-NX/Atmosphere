#pragma once
#include <switch.h>

#include "ipc_templating.hpp"

class IServiceObject {
    public:
        virtual ~IServiceObject() { }
        virtual Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) = 0;
        virtual Result handle_deferred() = 0;
};