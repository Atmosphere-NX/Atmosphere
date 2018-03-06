#include "log_device_null.h"

static void initialize(debug_log_device_null_t *this) {
    (void)this;
    /* Do nothing */
}

static void write_string(debug_log_device_null_t *this, const char *str, size_t len) {
    (void)this;
    (void)str;
    (void)len;
    /* Do nothing */
}

static void finalize(debug_log_device_null_t *this) {
    (void)this;
    /* Do nothing */
}

debug_log_device_null_t g_debug_log_device_null = {
    .super = {
        .initialize     = (void (*)(debug_log_device_t *, ...))initialize,
        .write_string   = (void (*)(debug_log_device_t *, const char *, size_t))write_string,
        .finalize       = (void (*)(debug_log_device_t *))finalize,
    },
};
