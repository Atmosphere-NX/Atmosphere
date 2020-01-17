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
#include "pinmux.h"
#include "gpio.h"
#include "car.h"
#include "../../irq.h"
#include "../../timer.h"

static uintptr_t g_uartRegBase;

static inline volatile tegra_uart_t *uartGetRegisters(UartDevice dev) 
{
    static const size_t offsets[] = { 0, 0x40, 0x200, 0x300, 0x400 };
    return (volatile tegra_uart_t *)(g_uartRegBase + offsets[dev]);
}

static inline void uartWaitCycles(u32 baud, u32 num)
{
    timerWaitUsecs((num * 1000000 + 16 * baud - 1) / (16 * baud));
}

static inline void uartWaitSyms(u32 baud, u32 num)
{
    timerWaitUsecs((num * 1000000 + baud - 1) / baud);
}

static void uartSetPinmuxConfig(UartDevice dev) {
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
            // Unused.
            break;
        default: break;
    }
}

static void uartReset(UartDevice dev)
{
    static const CarDevice uartCarDevs[] = { CARDEVICE_UARTA, CARDEVICE_UARTB, CARDEVICE_UARTC, CARDEVICE_UARTD };
    if (dev == UART_B) {
        gpio_configure_mode(TEGRA_GPIO(G, 0), GPIO_MODE_SFIO);
    } else {
        gpio_configure_mode(TEGRA_GPIO(G, 0), GPIO_MODE_GPIO);
    }

    if (dev == UART_C) {
        gpio_configure_mode(TEGRA_GPIO(D, 1), GPIO_MODE_SFIO);
    } else {
        gpio_configure_mode(TEGRA_GPIO(D, 1), GPIO_MODE_GPIO);
    }

    uartSetPinmuxConfig(dev);
    clkrst_reboot(uartCarDevs[dev]);
}


// This function blocks until the UART device is in the desired state.
static void uartWaitIdle(UartDevice dev, UartVendorStatus status)
{
    volatile tegra_uart_t *uart = uartGetRegisters(dev);

    if (status & UART_VENDOR_STATE_TX_IDLE) {
        while (!(uart->lsr & UART_LSR_TMTY));
    }

    if (status & UART_VENDOR_STATE_RX_IDLE) {
        while (uart->lsr & UART_LSR_RDR);
    }
}

void uartSetRegisterBase(uintptr_t regBase)
{
    g_uartRegBase = regBase;
}

void uartInit(UartDevice dev, u32 baud, u32 flags)
{
    volatile tegra_uart_t *uart = uartGetRegisters(dev);
    bool inverted = (flags & BIT(0)) != 0;

    // Set pinmux, gpio, clock
    uartReset(dev);

    // Wait for idle state.
    uartWaitIdle(dev, UART_VENDOR_STATE_TX_IDLE);

    // Calculate baud rate, round to nearest.
    u32 rate = (8 * baud + 408000000) / (16 * baud);

    uart->lcr &= ~UART_LCR_DLAB;                                // Disable DLAB.
    uart->ier = 0;                                              // Disable all interrupts.
    uart->mcr = 0;

    // Setup UART in FIFO mode
    uart->lcr = UART_LCR_DLAB | UART_LCR_WD_LENGTH_8;           // Enable DLAB and set word length 8.
    uart->dll = (u8)rate;                                       // Divisor latch LSB.
    uart->dlh = (u8)(rate >> 8);                                // Divisor latch MSB.
    uart->lcr &= ~UART_LCR_DLAB;                                // Disable DLAB.
    uart->spr;                                                  // Dummy read.
    uartWaitSyms(baud, 3);                                      // Wait for 3 symbols at the new baudrate.

    // Enable FIFO with default settings.
    uart->fcr = UART_FCR_FCR_EN_FIFO;
    uart->irda_csr = inverted ? UART_IRDA_CSR_INVERT_TXD : 0;   // Invert TX if needed
    uart->spr;                                                  // Dummy read as mandated by TRM.
    uartWaitCycles(baud, 3);                                    // Wait for 3 baud cycles, as mandated by TRM (erratum).

    // Flush FIFO.
    uartWaitIdle(dev, UART_VENDOR_STATE_TX_IDLE);               // Make sure there's no data being written in TX FIFO (TRM).
    uart->fcr |= UART_FCR_RX_CLR | UART_FCR_TX_CLR;             // Clear TX and RX FIFOs.
    uartWaitCycles(baud, 32);                                   // Wait for 32 baud cycles (TRM, erratum).

    // Wait for idle state (TRM).
    uartWaitIdle(dev, UART_VENDOR_STATE_TX_IDLE | UART_VENDOR_STATE_RX_IDLE);

    // Set scratch register to 0. We'll use it to backup write-only IER later
    uart->spr = 0;

    // Register the interrupt ID
    configureInterrupt(uartGetIrqId(dev), IRQ_PRIORITY_HOST, true);
}

void uartWriteData(UartDevice dev, const void *buffer, size_t size)
{
    volatile tegra_uart_t *uart = uartGetRegisters(dev);
    const u8 *buf8 = (const u8 *)buffer;

    for (size_t i = 0; i < size; i++) {
        while (!(uart->lsr & UART_LSR_THRE)); // Wait until it's possible to send data.
        uart->thr = buf8[i];
    }
}

void uartReadData(UartDevice dev, void *buffer, size_t size)
{
    volatile tegra_uart_t *uart = uartGetRegisters(dev);
    u8 *buf8 = (u8 *)buffer;

    for (size_t i = 0; i < size; i++) {
        while (!(uart->lsr & UART_LSR_RDR)) // Wait until it's possible to receive data.
        buf8[i] = uart->rbr;
    }
}

size_t uartReadDataMax(UartDevice dev, void *buffer, size_t maxSize)
{
    volatile tegra_uart_t *uart = uartGetRegisters(dev);
    u8 *buf8 = (u8 *)buffer;
    size_t count = 0;

    for (size_t i = 0; i < maxSize && (uart->lsr & UART_LSR_RDR); i++) {
        buf8[i] = uart->rbr;
        ++count;
    }

    return count;
}

size_t uartReadDataUntil(UartDevice dev, char *buffer, size_t maxSize, char delimiter)
{
    volatile tegra_uart_t *uart = uartGetRegisters(dev);
    size_t count = 0;

    for (size_t i = 0; i < maxSize && (uart->lsr & UART_LSR_RDR); i++) {
        while (!(uart->lsr & UART_LSR_RDR)) // Wait until it's possible to receive data.

        buffer[i] = uart->rbr;
        ++count;
        if (buffer[i] == delimiter) {
            break;
        }
    }

    return count;
}

ReadWriteDirection uartGetInterruptDirection(UartDevice dev)
{
    volatile tegra_uart_t *uart = uartGetRegisters(dev);
    u32 ret = 0;

    u32 iir = uart->iir & 0xF;

    if (iir == 8 || iir == 12) {
        // Data ready or data timeout
        ret |= DIRECTION_READ;
    } else if (iir == 2) {
        // TX FIFO empty
        ret |= DIRECTION_WRITE;
    }

    return (ReadWriteDirection)ret;
}

void uartSetInterruptStatus(UartDevice dev, ReadWriteDirection direction, bool enable)
{
    volatile tegra_uart_t *uart = uartGetRegisters(dev);

    u32 mask = 0;
    if (direction & DIRECTION_READ) {
        mask |= UART_IER_IE_RX_TIMEOUT | UART_IER_IE_RHR;
    }
    if (direction & DIRECTION_WRITE) {
        mask |= UART_IER_IE_THR;
    }

    if (enable) {
        uart->spr |= mask;
        uart->ier = uart->spr;
    } else {
        uart->spr &= ~mask;
        uart->ier = uart->spr;
    }
}
