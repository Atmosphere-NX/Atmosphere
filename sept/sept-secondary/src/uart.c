/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include "uart.h"
#include "timers.h"

void uart_init(UartDevice dev, uint32_t baud) {
    volatile tegra_uart_t *uart = uart_get_regs(dev);

    /* Set baud rate. */
    uint32_t rate = (8 * baud + 408000000) / (16 * baud);
    uart->UART_LCR = UART_LCR_DLAB; /* Enable DLAB. */
    uart->UART_THR_DLAB = (uint8_t)rate; /* Divisor latch LSB. */
    uart->UART_IER_DLAB = (uint8_t)(rate >> 8); /* Divisor latch MSB. */
    uart->UART_LCR = 0; /* Diable DLAB. */

    /* Setup UART in fifo mode. */
    uart->UART_IER_DLAB = 0;
    uart->UART_IIR_FCR = UART_FCR_FCR_EN_FIFO | UART_FCR_RX_CLR | UART_FCR_TX_CLR; /* Enable and clear TX and RX FIFOs. */
    (void)uart->UART_LSR;
    udelay(3 * ((baud + 999999) / baud));
    uart->UART_LCR = UART_LCR_WD_LENGTH_8; /* Set word length 8. */
    uart->UART_MCR = 0;
    uart->UART_MSR = 0;
    uart->UART_IRDA_CSR = 0;
    uart->UART_RX_FIFO_CFG = 1; /* Set RX_FIFO trigger level */
    uart->UART_MIE = 0;
    uart->UART_ASR = 0;
}

/* This function blocks until the UART device (dev) is in the desired state (status). Make sure the desired state can be reached! */
void uart_wait_idle(UartDevice dev, UartVendorStatus status) {
    while (!(uart_get_regs(dev)->UART_VENDOR_STATUS & status)) {
        /* Wait */
    }
}

void uart_send(UartDevice dev, const void *buf, size_t len) {
    volatile tegra_uart_t *uart = uart_get_regs(dev);

    for (size_t i = 0; i < len; i++) {
        while (uart->UART_LSR & UART_LSR_TX_FIFO_FULL) {
            /* Wait until the TX FIFO isn't full */
        }
        uart->UART_THR_DLAB = *((const uint8_t *)buf + i);
    }
}

void uart_recv(UartDevice dev, void *buf, size_t len) {
    volatile tegra_uart_t *uart = uart_get_regs(dev);

    for (size_t i = 0; i < len; i++) {
        while (uart->UART_LSR & UART_LSR_RX_FIFO_EMPTY) {
            /* Wait until the RX FIFO isn't empty */
        }
        *((uint8_t *)buf + i) = uart->UART_THR_DLAB;
    }
}
