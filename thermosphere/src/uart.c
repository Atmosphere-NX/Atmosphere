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
 
#include "timers.h"
#include "uart.h"
#include "misc.h"
#include "car.h"
#include "gpio.h"

static inline void uart_wait_cycles(u32 baud, u32 num)
{
    udelay((num * 1000000 + 16 * baud - 1) / (16 * baud));
}

static inline void uart_wait_syms(u32 baud, u32 num)
{
    udelay((num * 1000000 + baud - 1) / baud);
}

void uart_select(UartDevice dev)
{
    unsigned int id = (unsigned int)dev;
    PINMUX_AUX_UARTn_TX_0(id) = 0; /* UART */
    PINMUX_AUX_UARTn_RX_0(id) = 0x48; /* UART, enable, pull up */
    PINMUX_AUX_UARTn_RTS_0(id) = 0; /* UART */
    PINMUX_AUX_UARTn_CTS_0(id) = 0x44; /* UART, enable, pull down */
}

void uart_set_baudrate(UartDevice dev, u32 baud)
{
    //Set baud rate.
    volatile uart_t *uart = get_uart_device(dev);

    // Round to closest (hence the 8 * baud: +0.5)
    u32 rate = (8 * baud + UART_CLKRATE) / (16 * baud);

    // Enable DLAB
    uart->UART_LCR |= UART_LCR_DLAB;

    // Set divisor
    uart->UART_THR_DLAB = (u8)rate;
    uart->UART_IER_DLAB = (u8)(rate >> 8);

    // Disable DLAB
    uart->UART_LCR &= ~UART_LCR_DLAB;
    uart->UART_LSR;

    // Wait 2 characters worth of time
    uart_wait_syms(baud, 2);
}

void uart_flush_fifos(UartDevice dev, u32 baud, bool reset)
{
    volatile uart_t *uart = get_uart_device(dev);

    if (reset) {
        uart->UART_IIR_FCR &= ~UART_FCR_FCR_EN_FIFO;
        udelay(60); // From nvidia
        uart->UART_IIR_FCR |= UART_FCR_TX_CLR | UART_FCR_RX_CLR;
        uart->UART_IIR_FCR |= UART_FCR_FCR_EN_FIFO;
    } else {
        uart->UART_IIR_FCR |= UART_FCR_TX_CLR | UART_FCR_RX_CLR;
    }

    uart->UART_LSR;

    // Erratum fix
    uart_wait_cycles(baud, 32);
}

void uart_init(UartDevice dev, u32 baud, bool txInverted)
{
    volatile uart_t *uart = get_uart_device(dev);

    CarDevice uartCarDevs[] = { CARDEVICE_UARTA, CARDEVICE_UARTB, CARDEVICE_UARTC, CARDEVICE_UARTD };
    if (dev == UART_C) {
        gpio_configure_mode(TEGRA_GPIO(G, 0), GPIO_MODE_GPIO);      // Leave UART-B as GPIO
        gpio_configure_mode(TEGRA_GPIO(D, 1), GPIO_MODE_SDIO);      // Change UART-C to SPIO
        // Fixme other uart?
    }
    uart_select(dev);
    clkrst_reboot(uartCarDevs[dev]);

    uart->UART_MCR = 0;
    uart->UART_MSR = 0;
    uart->UART_RX_FIFO_CFG = 1; // Reset value
    uart->UART_MIE = 0;
    uart->UART_ASR = 0;

    // Enable FIFO, etc
    uart->UART_IIR_FCR =  UART_FCR_FCR_EN_FIFO | UART_FCR_TX_TRIG_FIFO_COUNT_GREATER_16 | UART_FCR_RX_TRIG_FIFO_COUNT_GREATER_1;
    uart->UART_IRDA_CSR = txInverted ? 2 : 0;
    uart->UART_LSR;

    // Erratum fix
    uart_wait_cycles(baud, 3);

    // Set baud rate, etc.
    uart->UART_LCR = UART_LCR_WD_LENGTH_8;
    uart_set_baudrate(dev, baud);

    uart_flush_fifos(dev, baud, true);
}

// This function blocks until the UART device (dev) is in the desired state (status). Make sure the desired state can be reached!
void uart_wait_idle(UartDevice dev, UartVendorStatus status)
{
    while (!(get_uart_device(dev)->UART_VENDOR_STATUS & status));
}

void uart_send(UartDevice dev, const void *buf, size_t len)
{
    volatile uart_t *uart = get_uart_device(dev);
    const u8 *buf8 = (const u8 *)buf;

    for (size_t i = 0; i < len; i++) {
        while (uart->UART_LSR & UART_LSR_TX_FIFO_FULL);
        uart->UART_THR_DLAB = buf8[i];
    }
}

void uart_recv(UartDevice dev, void *buf, size_t len)
{
    volatile uart_t *uart = get_uart_device(dev);
    u8 *buf8 = (u8 *)buf;

    for (size_t i = 0; i < len; i++) {
        while (uart->UART_LSR & UART_LSR_RX_FIFO_EMPTY);
        buf8[i] = uart->UART_THR_DLAB;
    }
}

size_t uart_recv_max(UartDevice dev, void *buf, size_t maxlen)
{
    volatile uart_t *uart = get_uart_device(dev);
    u8 *buf8 = (u8 *)buf;
    size_t i;

    for (i = 0; i < maxlen && !(uart->UART_LSR & UART_LSR_RX_FIFO_EMPTY); i++) {
        buf8[i] = uart->UART_THR_DLAB;
    }

    return i;
}
