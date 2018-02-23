#ifndef EXOSPHERE_UART_H
#define EXOSPHERE_UART_H

#include <stdint.h>
#include "mmu.h"

/* Exosphere driver for the Tegra X1 UARTs. */

/* TODO: Should we bother with support UARTB-D? */

#define UARTA_BASE  (mmio_get_device_address(MMIO_DEVID_UART_A))

#define UART_THR_DLAB_0_0 (*((volatile uint32_t *)(UARTA_BASE + 0x0)))
#define UART_IER_DLAB_0_0 (*((volatile uint32_t *)(UARTA_BASE + 0x4)))
#define UART_IIR_FCR_0 (*((volatile uint32_t *)(UARTA_BASE+ 0x8)))
#define UART_LCR_0 (*((volatile uint32_t *)(UARTA_BASE + 0xC)))
#define UART_LSR_0 (*((volatile uint32_t *)(UARTA_BASE + 0x14)))

void uart_initialize(uint16_t divider);
void uart_transmit_char(char ch);
void uart_transmit_str(const char *str);
void uart_transmit_hex(uint32_t value);

#endif