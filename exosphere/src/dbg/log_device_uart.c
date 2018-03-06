#include "log_device_uart.h"
#include "../car.h"
#include "../uart.h"

static void initialize(debug_log_device_uart_t *this) {
    if (!this->is_initialized) {
        uart_select(0); /* UART-A */
        clkrst_enable(CARDEVICE_UARTA);
        uart_initialize(0 /* I don't know */);
        this->is_initialized = true;
    }
}

static void write_string(debug_log_device_uart_t *this, const char *str, size_t len) {
    (void)this;
    (void)len;
    uart_transmit_str(str);
}

static void finalize(debug_log_device_uart_t *this) {
    clkrst_disable(CARDEVICE_UARTA);
}

debug_log_device_uart_t g_debug_log_device_uart = {
    .super = {
        .initialize     = (void (*)(debug_log_device_t *, ...))initialize,
        .write_string   = (void (*)(debug_log_device_t *, const char *, size_t))write_string,
        .finalize       = (void (*)(debug_log_device_t *))finalize,
    },
};
