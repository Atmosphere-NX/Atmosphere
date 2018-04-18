#pragma once
#include <switch.h>

class IServiceObject {
    public:
        virtual void dispatch(IpcParsedCommand *r, u32 *cmd_buf, u32 cmd_id, u32 *in_rawdata, u32 in_rawdata_size, u32 *out_rawdata, u32 *out_raw_data_count);
};