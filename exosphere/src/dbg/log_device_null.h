#ifndef EXOSPHERE_DBG_LOG_DEVICE_NULL_H
#define EXOSPHERE_DBG_LOG_DEVICE_NULL_H

#include "log.h"

typedef struct {
    debug_log_device_t super;
    /* Additonnal attributes go here */
} debug_log_device_null_t;

extern debug_log_device_null_t g_debug_log_device_null;

#endif
