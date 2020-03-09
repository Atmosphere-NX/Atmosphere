/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "pinmux.h"

static inline void uart_wait_cycles(uint32_t baud, uint32_t num)
{
    udelay((num * 1000000 + 16 * baud - 1) / (16 * baud));
}

static inline void uart_wait_syms(uint32_t baud, uint32_t num)
{
    udelay((num * 1000000 + baud - 1) / baud);
}

void uart_config(UartDevice dev) {
    volatile tegra_pinmux_t *pinmux = pinmux_get_regs();

    switch (dev) {
        case UART_A:
            pinmux->uart1_tx = (0 | 0 | PINMUX_PULL_NONE | PINMUX_SELECT_FUNCTION0);
            pinmux->uart1_rx = (PINMUX_INPUT | PINMUX_TRISTATE | PINMUX_PULL_UP | PINMUX_SELECT_FUNCTION0);
            pinmux->uart1_rts = (0 | 0 | PINMUX_PULL_NONE | PINMUX_SELECT_FUNCTION0);
            pinmux->uart1_cts = (PINMUX_INPUT | PINMUX_TRISTATE | PINMUX_PULL_DOWN | PINMUX_SELECT_FUNCTION0);
            break;
        case UART_B:
            pinmux->uart2_tx = (0 | 0 | PINMUX_PULL_NONE | PINMUX_SELECT_FUNCTION0);
            pinmux->uart2_rx = (PINMUX_INPUT | PINMUX_TRISTATE | PINMUX_PULL_NONE | PINMUX_SELECT_FUNCTION0);
            pinmux->uart2_rts = (0 | 0 | PINMUX_PULL_DOWN | PINMUX_SELECT_FUNCTION0);
            pinmux->uart2_cts = (PINMUX_INPUT | PINMUX_TRISTATE | PINMUX_PULL_NONE | PINMUX_SELECT_FUNCTION0);
            break;
        case UART_C:
            pinmux->uart3_tx = (0 | 0 | PINMUX_PULL_NONE | PINMUX_SELECT_FUNCTION0);
            pinmux->uart3_rx = (PINMUX_INPUT | PINMUX_TRISTATE | PINMUX_PULL_NONE | PINMUX_SELECT_FUNCTION0);
            pinmux->uart3_rts = (0 | 0 | PINMUX_PULL_DOWN | PINMUX_SELECT_FUNCTION0);
            pinmux->uart3_cts = (PINMUX_INPUT | PINMUX_TRISTATE | PINMUX_PULL_NONE | PINMUX_SELECT_FUNCTION0);
            break;
        case UART_D:
            pinmux->uart4_tx = (0 | 0 | PINMUX_PULL_DOWN | PINMUX_SELECT_FUNCTION0);
            pinmux->uart4_rx = (PINMUX_INPUT | PINMUX_TRISTATE | PINMUX_PULL_DOWN | PINMUX_SELECT_FUNCTION0);
            pinmux->uart4_rts = (0 | 0 | PINMUX_PULL_DOWN | PINMUX_SELECT_FUNCTION0);
            pinmux->uart4_cts = (PINMUX_INPUT | PINMUX_TRISTATE | PINMUX_PULL_DOWN | PINMUX_SELECT_FUNCTION0);
            break;
        case UART_E:
            /* Unused. */
            break;
        default: break;
    }
}

void uart_init(UartDevice dev, uint32_t baud) {
    volatile tegra_uart_t *uart = uart_get_regs(dev);

    /* Wait for idle state. */
    uart_wait_idle(dev, UART_VENDOR_STATE_TX_IDLE);

    /* Calculate baud rate, round to nearest. */
    uint32_t rate = (8 * baud + 408000000) / (16 * baud);

    /* Setup UART in FIFO mode. */
    uart->UART_IER_DLAB = 0;
    uart->UART_MCR = 0;
    uart->UART_LCR = (UART_LCR_DLAB | UART_LCR_WD_LENGTH_8);        /* Enable DLAB and set word length 8. */
    uart->UART_THR_DLAB = (uint8_t)rate;                            /* Divisor latch LSB. */
    uart->UART_IER_DLAB = (uint8_t)(rate >> 8);                     /* Divisor latch MSB. */
    uart->UART_LCR &= ~(UART_LCR_DLAB);                             /* Disable DLAB. */
    uart->UART_SPR;                                                 /* Dummy read. */
    uart_wait_syms(baud, 3);                                        /* Wait for 3 symbols at the new baudrate. */

    /* Enable FIFO with default settings. */
    uart->UART_IIR_FCR = UART_FCR_FCR_EN_FIFO;
    uart->UART_SPR;                                                 /* Dummy read as mandated by TRM. */
    uart_wait_cycles(baud, 3);                                      /* Wait for 3 baud cycles, as mandated by TRM (erratum). */

    /* Flush FIFO. */
    uart_wait_idle(dev, UART_VENDOR_STATE_TX_IDLE);                 /* Make sure there's no data being written in TX FIFO (TRM). */
    uart->UART_IIR_FCR |= UART_FCR_RX_CLR | UART_FCR_TX_CLR;        /* Clear TX and RX FIFOs. */
    uart_wait_cycles(baud, 32);                                     /* Wait for 32 baud cycles (TRM, erratum). */
    /* Wait for idle state (TRM). */
    uart_wait_idle(dev, UART_VENDOR_STATE_TX_IDLE | UART_VENDOR_STATE_RX_IDLE);
}

/* This function blocks until the UART device is in the desired state. */
void uart_wait_idle(UartDevice dev, UartVendorStatus status) {
    volatile tegra_uart_t *uart = uart_get_regs(dev);
    
    if (status & UART_VENDOR_STATE_TX_IDLE) {
        while (!(uart->UART_LSR & UART_LSR_TMTY)) {
            /* Wait */
        }
    }
    if (status & UART_VENDOR_STATE_RX_IDLE) {
        while (uart->UART_LSR & UART_LSR_RDR) {
            /* Wait */
        }
    }
}

void uart_send(UartDevice dev, const void *buf, size_t len) {
    volatile tegra_uart_t *uart = uart_get_regs(dev);

    for (size_t i = 0; i < len; i++) {
        while (!(uart->UART_LSR & UART_LSR_THRE)) {
            /* Wait until it's possible to send data. */
        }
        uart->UART_THR_DLAB = *((const uint8_t *)buf + i);
    }
}

void uart_recv(UartDevice dev, void *buf, size_t len) {
    volatile tegra_uart_t *uart = uart_get_regs(dev);

    for (size_t i = 0; i < len; i++) {
        while (!(uart->UART_LSR & UART_LSR_RDR)) {
            /* Wait until it's possible to receive data. */
        }
        *((uint8_t *)buf + i) = uart->UART_THR_DLAB;
    }
}
