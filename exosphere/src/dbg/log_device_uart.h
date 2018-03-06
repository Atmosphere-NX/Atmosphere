#ifndef EXOSPHERE_DBG_LOG_DEVICE_UART_H
#define EXOSPHERE_DBG_LOG_DEVICE_UART_H

#include "log.h"

typedef struct {
    debug_log_device_t super;
    bool is_initialized;
} debug_log_device_uart_t;

extern debug_log_device_uart_t g_debug_log_device_uart;

#endif
