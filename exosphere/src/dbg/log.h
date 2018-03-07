#ifndef EXOSPHERE_DBG_LOG_H
#define EXOSPHERE_DBG_LOG_H

#include "../utils.h"

#define DBG_LOG_BUF_SIZE    256

typedef enum {
    DEBUGLOGDEVICE_NULL = 0,
    DEBUGLOGDEVICE_UART = 1,

    DEBUGLOGDEVICE_MAX = 2,
} DebugLogDevice;

typedef struct debug_log_device_t {
    void (*initialize)(struct debug_log_device_t *this, ...);
    void (*write_string)(struct debug_log_device_t *this, const char *str, size_t len);
    void (*finalize)(struct debug_log_device_t *this);
} debug_log_device_t;

void dbg_log_initialize(DebugLogDevice device);
void dbg_log_write(const char *fmt, ...);
void dbg_log_finalize(void);

#endif
