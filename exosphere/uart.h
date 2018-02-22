#ifndef EXOSPHERE_UART_H
#define EXOSPHERE_UART_H

#include <stdint.h>

/* Exosphere driver for the Tegra X1 UARTs. */

void set_uart_address(void *uart_base);
void *get_uart_address(void); /* This is inlined in uart.c */

#define UART_THR_DLAB_0_0 (*((volatile uint32_t *)(get_uart_address() + 0x0)))
#define UART_IER_DLAB_0_0 (*((volatile uint32_t *)(get_uart_address() + 0x4)))
#define UART_IIR_FCR_0 (*((volatile uint32_t *)(get_uart_address() + 0x8)))
#define UART_LCR_0 (*((volatile uint32_t *)(get_uart_address() + 0xC)))
#define UART_LSR_0 (*((volatile uint32_t *)(get_uart_address() + 0x14)))

void uart_initialize(uint16_t divider);
void uart_transmit_char(char ch);
void uart_transmit_str(const char *str);
void uart_transmit_hex(uint32_t value);

#endif