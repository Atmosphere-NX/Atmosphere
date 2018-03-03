#ifndef EXOSPHERE_UART_H
#define EXOSPHERE_UART_H

#include <stdint.h>
#include "memory_map.h"

/* Exosphere driver for the Tegra X1 UARTs. */

/* TODO: Should we bother with support UARTB-D? */

static inline uintptr_t get_uarta_base(void) {
    return MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_UART_A);
}

#define UARTA_BASE  (get_uarta_base())

#define UART_THR_DLAB_0_0 MAKE_REG32(UARTA_BASE + 0x0)
#define UART_IER_DLAB_0_0 MAKE_REG32(UARTA_BASE + 0x4)
#define UART_IIR_FCR_0 MAKE_REG32(UARTA_BASE+ 0x8)
#define UART_LCR_0 MAKE_REG32(UARTA_BASE + 0xC)
#define UART_LSR_0 MAKE_REG32(UARTA_BASE + 0x14)

void uart_initialize(uint16_t divider);
void uart_transmit_char(char ch);
void uart_transmit_str(const char *str);
void uart_transmit_hex(uint32_t value);

#endif
