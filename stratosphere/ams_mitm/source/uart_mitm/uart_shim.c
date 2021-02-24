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

#include <string.h>
#include <switch.h>
#include "uart_shim.h"

/* Command forwarders. */
Result uartCreatePortSessionFwd(Service* s, UartPortSession* out) {
    return serviceDispatch(s, 6,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

// [7.0.0+]
static Result _uartPortSessionOpenPortFwd(UartPortSession* s, bool *out, u32 port, u32 baud_rate, UartFlowControlMode flow_control_mode, u32 device_variation, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, Handle send_handle, Handle receive_handle, u64 send_buffer_length, u64 receive_buffer_length, u32 cmd_id) {
    const struct {
        u8 is_invert_tx;
        u8 is_invert_rx;
        u8 is_invert_rts;
        u8 is_invert_cts;
        u32 port;
        u32 baud_rate;
        u32 flow_control_mode;
        u32 device_variation;
        u32 pad;
        u64 send_buffer_length;
        u64 receive_buffer_length;
    } in = { is_invert_tx!=0, is_invert_rx!=0, is_invert_rts!=0, is_invert_cts!=0, port, baud_rate, flow_control_mode, device_variation, 0, send_buffer_length, receive_buffer_length };

    u8 tmp=0;
    Result rc = serviceDispatchInOut(&s->s, cmd_id, in, tmp,
        .in_num_handles = 2,
        .in_handles = { send_handle, receive_handle },
    );
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result uartPortSessionOpenPortFwd(UartPortSession* s, bool *out, u32 port, u32 baud_rate, UartFlowControlMode flow_control_mode, u32 device_variation, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, Handle send_handle, Handle receive_handle, u64 send_buffer_length, u64 receive_buffer_length) {
    return _uartPortSessionOpenPortFwd(s, out, port, baud_rate, flow_control_mode, device_variation, is_invert_tx, is_invert_rx, is_invert_rts, is_invert_cts, send_handle, receive_handle, send_buffer_length, receive_buffer_length, 0);
}

Result uartPortSessionOpenPortForDevFwd(UartPortSession* s, bool *out, u32 port, u32 baud_rate, UartFlowControlMode flow_control_mode, u32 device_variation, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, Handle send_handle, Handle receive_handle, u64 send_buffer_length, u64 receive_buffer_length) {
    return _uartPortSessionOpenPortFwd(s, out, port, baud_rate, flow_control_mode, device_variation, is_invert_tx, is_invert_rx, is_invert_rts, is_invert_cts, send_handle, receive_handle, send_buffer_length, receive_buffer_length, 1);
}

Result uartPortSessionBindPortEventFwd(UartPortSession* s, UartPortEventType port_event_type, s64 threshold, bool *out, Handle *handle_out) {
    const struct {
        u32 port_event_type;
        u32 pad;
        u64 threshold;
    } in = { port_event_type, 0, threshold };

    u8 tmp=0;
    Result rc = serviceDispatchInOut(&s->s, 6, in, tmp,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = handle_out,
    );
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

