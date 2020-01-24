/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    Handle target_session;
    u32 context;

    SfBufferAttrs buffer_attrs;
    SfBuffer buffers[8];

    bool in_send_pid;

    u32 in_num_objects;
    const Service* in_objects[8];

    u32 in_num_handles;
    Handle in_handles[8];

    u32 out_num_objects;
    Service* out_objects;

    SfOutHandleAttrs out_handle_attrs;
    Handle* out_handles;

    u64 override_pid;
} SfMitmDispatchParams;

NX_INLINE Result serviceMitmDispatchImpl(
    Service* s, u32 request_id,
    const void* in_data, u32 in_data_size,
    void* out_data, u32 out_data_size,
    SfMitmDispatchParams disp
)
{
    // Make a copy of the service struct, so that the compiler can assume that it won't be modified by function calls.
    Service srv = *s;

    void* in = serviceMakeRequest(&srv, request_id, disp.context,
        in_data_size, disp.in_send_pid,
        disp.buffer_attrs, disp.buffers,
        disp.in_num_objects, disp.in_objects,
        disp.in_num_handles, disp.in_handles);

    if (in_data_size)
        __builtin_memcpy(in, in_data, in_data_size);

    if (disp.in_send_pid && disp.override_pid) {
        const u64 pid = (disp.override_pid & 0x0000FFFFFFFFFFFFul) | 0xFFFE000000000000ul;
        __builtin_memcpy((u8 *)armGetTls() + 0xC, &pid, sizeof(pid));
    }

    Result rc = svcSendSyncRequest(disp.target_session == INVALID_HANDLE ? s->session : disp.target_session);
    if (R_SUCCEEDED(rc)) {
        void* out = NULL;
        rc = serviceParseResponse(&srv,
            out_data_size, &out,
            disp.out_num_objects, disp.out_objects,
            disp.out_handle_attrs, disp.out_handles);

        if (R_SUCCEEDED(rc) && out_data && out_data_size)
            __builtin_memcpy(out_data, out, out_data_size);
    }

    return rc;
}

#define serviceMitmDispatch(_s,_rid,...) \
    serviceMitmDispatchImpl((_s),(_rid),NULL,0,NULL,0,(SfMitmDispatchParams){ __VA_ARGS__ })

#define serviceMitmDispatchIn(_s,_rid,_in,...) \
    serviceMitmDispatchImpl((_s),(_rid),&(_in),sizeof(_in),NULL,0,(SfMitmDispatchParams){ __VA_ARGS__ })

#define serviceMitmDispatchOut(_s,_rid,_out,...) \
    serviceMitmDispatchImpl((_s),(_rid),NULL,0,&(_out),sizeof(_out),(SfMitmDispatchParams){ __VA_ARGS__ })

#define serviceMitmDispatchInOut(_s,_rid,_in,_out,...) \
    serviceMitmDispatchImpl((_s),(_rid),&(_in),sizeof(_in),&(_out),sizeof(_out),(SfMitmDispatchParams){ __VA_ARGS__ })

#ifdef __cplusplus
}
#endif
