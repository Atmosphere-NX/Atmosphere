/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "gpio.h"
#include "car.h"

void uart_config(UartDevice dev) {
    volatile tegra_pinmux_t *pinmux = pinmux_get_regs();
    
    switch (dev) {
        case UART_A:
            pinmux->uart1_tx = 0;
            pinmux->uart1_rx = (PINMUX_INPUT | PINMUX_PULL_UP);
            pinmux->uart1_rts = 0;
            pinmux->uart1_cts = (PINMUX_INPUT | PINMUX_PULL_DOWN);
            break;
        case UART_B:
            pinmux->uart2_tx = 0;
            pinmux->uart2_rx = (PINMUX_INPUT | PINMUX_PULL_UP);
            pinmux->uart2_rts = 0;
            pinmux->uart2_cts = (PINMUX_INPUT | PINMUX_PULL_DOWN);
            break;
        case UART_C:
            pinmux->uart3_tx = 0;
            pinmux->uart3_rx = (PINMUX_INPUT | PINMUX_PULL_UP);
            pinmux->uart3_rts = 0;
            pinmux->uart3_cts = (PINMUX_INPUT | PINMUX_PULL_DOWN);
            break;
        case UART_D:
            pinmux->uart4_tx = 0;
            pinmux->uart4_rx = (PINMUX_INPUT | PINMUX_PULL_UP);
            pinmux->uart4_rts = 0;
            pinmux->uart4_cts = (PINMUX_INPUT | PINMUX_PULL_DOWN);
            break;
        case UART_E:
            /* Unused. */
            break;
        default: break;
    }
}

void uart_reset(UartDevice dev)
{
    CarDevice uartCarDevs[] = { CARDEVICE_UARTA, CARDEVICE_UARTB, CARDEVICE_UARTC, CARDEVICE_UARTD };
    if (dev == UART_C) {
        gpio_configure_mode(TEGRA_GPIO(G, 0), GPIO_MODE_GPIO);      // Leave UART-B as GPIO
        gpio_configure_mode(TEGRA_GPIO(D, 1), GPIO_MODE_SFIO);      // Change UART-C to SPIO
        // Fixme other uart?
    }
    clkrst_reboot(uartCarDevs[dev]);
}

void uart_init(UartDevice dev, uint32_t baud, bool inverted) {
    volatile tegra_uart_t *uart = uart_get_regs(dev);

    /* Wait for idle state. */
    uart_wait_idle(dev, UART_VENDOR_STATE_TX_IDLE);

    /* Calculate baud rate. */
    uint32_t rate = (8 * baud + 408000000) / (16 * baud);

    /* Setup UART in FIFO mode. */
    uart->UART_IER_DLAB = 0;
    uart->UART_MCR = 0;
    uart->UART_LCR = (UART_LCR_DLAB | UART_LCR_WD_LENGTH_8);        /* Enable DLAB and set word length 8. */
    uart->UART_THR_DLAB = (uint8_t)rate;                            /* Divisor latch LSB. */
    uart->UART_IER_DLAB = (uint8_t)(rate >> 8);                     /* Divisor latch MSB. */
    uart->UART_LCR &= ~(UART_LCR_DLAB);                             /* Disable DLAB. */
    
    /* Flush FIFO. */
    uart->UART_IIR_FCR = (UART_FCR_FCR_EN_FIFO | UART_FCR_RX_CLR | UART_FCR_TX_CLR);    /* Enable and clear TX and RX FIFOs. */
    uart->UART_IRDA_CSR = inverted ? 2 : 0; /* Invert TX */
    udelay(3 * ((baud + 999999) / baud));
    
    /* Wait for idle state. */
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

size_t uart_recv_max(UartDevice dev, void *buf, size_t max_len) {
    volatile tegra_uart_t *uart = uart_get_regs(dev);
    size_t i;

    for (i = 0; i < max_len && (uart->UART_LSR & UART_LSR_RDR); i++) {
        *((uint8_t *)buf + i) = uart->UART_THR_DLAB;
    }

    return 1 + i;
}