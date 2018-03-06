#include "uart.h"
#include "misc.h"

void uart_select(unsigned int id) {
    /* This confirmation is valid for UART-A, I don't know about the other UARTs. */
    /* Official Nintendo code (for UART-A, at least) */
    PINMUX_AUX_UARTn_TX_0(id) = 0; /* UART */
    PINMUX_AUX_UARTn_RX_0(id) = 0x48; /* UART, enable, pull up */
    PINMUX_AUX_UARTn_RTS_0(id) = 0; /* UART */
    PINMUX_AUX_UARTn_CTS_0(id) = 0x44; /* UART, enable, pull down */
}

void uart_initialize(uint16_t divider) {
    /* Setup UART in 16450 mode. We assume the relevant UART clock has been enabled. */

    /* Disable FIFO */
    UART_IIR_FCR_0 = 0x00;

    /* Set DLAB */
    UART_LCR_0 = 0x80;
    UART_THR_DLAB_0_0 = (uint8_t)divider;
    UART_IER_DLAB_0_0 = (uint8_t)(divider >> 8);

    /* 8N1 mode */
    UART_LCR_0 = 0x03;
}

void uart_transmit_char(char ch) {
    /* Wait for THR to be empty */
    while (!(UART_LSR_0 & 0x20)) {}

    UART_THR_DLAB_0_0 = ch;
}

void uart_transmit_str(const char *str) {
    while (*str) {
        uart_transmit_char(*str++);
    }
}

void uart_transmit_hex(uint32_t value) {
    for (unsigned int i = 0; i < 8; i++) {
        uint32_t nibble = (value >> (28 - i * 4)) & 0xF;
        uart_transmit_char("0123456789ABCDEF"[nibble]);
    }
}
