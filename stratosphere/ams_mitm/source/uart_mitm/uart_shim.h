/**
 * @file uart_shim.h
 * @brief UART IPC wrapper.
 * @author yellows8
 * @copyright libnx Authors
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Command forwarders. */
Result uartCreatePortSessionFwd(Service* s, UartPortSession* out);

Result uartPortSessionOpenPortFwd(UartPortSession* s, bool *out, u32 port, u32 baud_rate, UartFlowControlMode flow_control_mode, u32 device_variation, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, Handle send_handle, Handle receive_handle, u64 send_buffer_length, u64 receive_buffer_length);
Result uartPortSessionOpenPortForDevFwd(UartPortSession* s, bool *out, u32 port, u32 baud_rate, UartFlowControlMode flow_control_mode, u32 device_variation, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, Handle send_handle, Handle receive_handle, u64 send_buffer_length, u64 receive_buffer_length);
Result uartPortSessionBindPortEventFwd(UartPortSession* s, UartPortEventType port_event_type, s64 threshold, bool *out, Handle *handle_out);

#ifdef __cplusplus
}
#endif
