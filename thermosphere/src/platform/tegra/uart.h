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
 
#pragma once

#include "../../utils.h"
#include "interrupt_config.h"

/* UART devices */
typedef enum UartDevice {
    UART_A = 0,
    UART_B = 1,
    UART_C = 2,
    UART_D = 3,
    UART_E = 4,

    UART_MAX = UART_E, // Treat UART_E as if it didn't exist
} UartDevice;

/* 36.3.12 UART_VENDOR_STATUS_0_0 */
typedef enum {
    UART_VENDOR_STATE_TX_IDLE = 1 << 0,
    UART_VENDOR_STATE_RX_IDLE = 1 << 1,

    /* This bit is set to 1 when a read is issued to an empty FIFO and gets cleared on register read (sticky bit until read)
    0 = NO_UNDERRUN
    1 = UNDERRUN
    */
    UART_VENDOR_STATE_RX_UNDERRUN = 1 << 2,

    /* This bit is set to 1 when write data is issued to the TX FIFO when it is already full and gets cleared on register read (sticky bit until read)
    0 = NO_OVERRUN
    1 = OVERRUN
    */
    UART_VENDOR_STATE_TX_OVERRUN = 1 << 3,

    UART_VENDOR_STATE_RX_FIFO_COUNTER = 0b111111 << 16, /* reflects number of current entries in RX FIFO */
    UART_VENDOR_STATE_TX_FIFO_COUNTER = 0b111111 << 24 /* reflects number of current entries in TX FIFO */
} UartVendorStatus;

/* 36.3.6 UART_LSR_0 */
typedef enum {
    UART_LSR_RDR           = 1 << 0, /* Receiver Data Ready */
    UART_LSR_OVRF          = 1 << 1, /* Receiver Overrun Error */
    UART_LSR_PERR          = 1 << 2, /* Parity Error */
    UART_LSR_FERR          = 1 << 3, /* Framing Error */
    UART_LSR_BRK           = 1 << 4, /* BREAK condition detected on line */
    UART_LSR_THRE          = 1 << 5, /* Transmit Holding Register is Empty -- OK to write data */
    UART_LSR_TMTY          = 1 << 6, /* Transmit Shift Register empty status */
    UART_LSR_FIFOE         = 1 << 7, /* Receive FIFO Error */
    UART_LSR_TX_FIFO_FULL  = 1 << 8, /* Transmitter FIFO full status */
    UART_LSR_RX_FIFO_EMPTY = 1 << 9, /* Receiver FIFO empty status */
} UartLineStatus;

/* 36.3.4 UART_LCR_0 */
typedef enum {
    UART_LCR_WD_LENGTH_5 = 0, /* word length 5 */
    UART_LCR_WD_LENGTH_6 = 1, /* word length 6 */
    UART_LCR_WD_LENGTH_7 = 2, /* word length 7 */
    UART_LCR_WD_LENGTH_8 = 3, /* word length 8 */

    /* STOP:
    0 = Transmit 1 stop bit
    1 = Transmit 2 stop bits (receiver always checks for 1 stop bit)
    */
    UART_LCR_STOP        = 1 << 2, 
    UART_LCR_PAR         = 1 << 3, /* Parity enabled */
    UART_LCR_EVEN        = 1 << 4, /* Even parity format. There will always be an even number of 1s in the binary representation (PAR = 1) */
    UART_LCR_SET_P       = 1 << 5, /* Set (force) parity to value in LCR[4] */
    UART_LCR_SET_B       = 1 << 6, /* Set BREAK condition -- Transmitter sends all zeroes to indicate BREAK */
    UART_LCR_DLAB        = 1 << 7, /* Divisor Latch Access Bit (set to allow programming of the DLH, DLM Divisors) */
} UartLineControl;

/* 36.3.3 UART_IIR_FCR_0 */
typedef enum {
    UART_FCR_FCR_EN_FIFO = 1 << 0, /* Enable the transmit and receive FIFOs. This bit should be enabled */
    UART_FCR_RX_CLR      = 1 << 1, /* Clears the contents of the receive FIFO and resets its counter logic to 0 (the receive shift register is not cleared or altered). This bit returns to 0 after clearing the FIFOs */
    UART_FCR_TX_CLR      = 1 << 2, /* Clears the contents of the transmit FIFO and resets its counter logic to 0 (the transmit shift register is not cleared or altered). This bit returns to 0 after clearing the FIFOs */

    /* DMA:
    0 = DMA_MODE_0
    1 = DMA_MODE_1
    */
    UART_FCR_DMA         = 1 << 3,

    /* TX_TRIG
    0 = FIFO_COUNT_GREATER_16
    1 = FIFO_COUNT_GREATER_8
    2 = FIFO_COUNT_GREATER_4
    3 = FIFO_COUNT_GREATER_1
    */
    UART_FCR_TX_TRIG                       = 3 << 4,
    UART_FCR_TX_TRIG_FIFO_COUNT_GREATER_16 = 0 << 4,
    UART_FCR_TX_TRIG_FIFO_COUNT_GREATER_8  = 1 << 4,
    UART_FCR_TX_TRIG_FIFO_COUNT_GREATER_4  = 2 << 4,
    UART_FCR_TX_TRIG_FIFO_COUNT_GREATER_1  = 3 << 4,

    /* RX_TRIG
    0 = FIFO_COUNT_GREATER_1
    1 = FIFO_COUNT_GREATER_4
    2 = FIFO_COUNT_GREATER_8
    3 = FIFO_COUNT_GREATER_16 
    */
    UART_FCR_RX_TRIG                       = 3 << 6,
    UART_FCR_RX_TRIG_FIFO_COUNT_GREATER_1  = 0 << 6,
    UART_FCR_RX_TRIG_FIFO_COUNT_GREATER_4  = 1 << 6,
    UART_FCR_RX_TRIG_FIFO_COUNT_GREATER_8  = 2 << 6,
    UART_FCR_RX_TRIG_FIFO_COUNT_GREATER_16 = 3 << 6,
} UartFifoControl;

/* 36.3.2 UART_IER_DLAB_0_0 */
typedef enum {
    UART_IER_IE_RHR         = 1 << 0, /* Interrupt enable for Received Data Interrupt */
    UART_IER_IE_THR         = 1 << 1, /* Interrupt enable for Transmitter Holding Register Empty interrupt */
    UART_IER_IE_RXS         = 1 << 2, /* Interrupt enable for Receiver Line Status Interrupt */
    UART_IER_IE_MSI         = 1 << 3, /* Interrupt enable for Modem Status Interrupt */
    UART_IER_IE_RX_TIMEOUT  = 1 << 4, /* Interrupt enable for RX FIFO timeout */
    UART_IER_IE_EORD        = 1 << 5, /* Interrupt enable for Interrupt Enable for End of Received Data */

} UartInterruptEnable;

/* 36.3.3 UART_IIR_FCR_0 */
typedef enum {
    UART_IIR_IS_STA  = 1 << 0, /* Interrupt Pending if ZERO */
    UART_IIR_IS_PRI0 = 1 << 1, /* Encoded Interrupt ID Refer to IIR[3:0] table [36.3.3] */
    UART_IIR_IS_PRI1 = 1 << 2, /* Encoded Interrupt ID Refer to IIR[3:0] table */
    UART_IIR_IS_PRI2 = 1 << 3, /* Encoded Interrupt ID Refer to IIR[3:0] table */

    /* FIFO Mode Status
    0 = 16450 mode (no FIFO)
    1 = 16550 mode (FIFO)
    */
    UART_IIR_EN_FIFO    = 3 << 6,
    UART_IIR_MODE_16450 = 0 << 6,
    UART_IIR_MODE_16550 = 1 << 6,
} UartInterruptIdentification;

/* 36.3.9 UART_IRDA_CSR_0 */
typedef enum {
    UART_IRDA_CSR_INVERT_RXD                = 1 << 0,
    UART_IRDA_CSR_INVERT_TXD                = 1 << 1,
    UART_IRDA_CSR_INVERT_CTS                = 1 << 2,
    UART_IRDA_CSR_INVERT_RTS                = 1 << 3,

    UART_IRDA_CSR_PWT_A_BAUD_PULSE_3_14     = 0 << 6,
    UART_IRDA_CSR_PWT_A_BAUD_PULSE_4_14     = 1 << 6,
    UART_IRDA_CSR_SIR_A                     = 1 << 7,
} UartIrDAPulseCodingCSR;

typedef struct {
    union {
        // UART_THR_DLAB_0
        u32 thr;
        u32 rbr;
        u32 dll;
    };
    union {
        // UART_IER_DLAB_0
        u32 ier;
        u32 dlh;
    };
    union {
        // UART_IIR_FCR_0
        u32 iir;
        u32 fcr;
    };
    u32 lcr;
    u32 mcr;
    u32 lsr;
    u32 msr;
    u32 spr;
    u32 irda_csr;
    u32 rx_fifo_cfg;
    u32 mie;
    u32 vendor_status;
    u8 _0x30[0x0C];
    u32 asr;
} tegra_uart_t;

void uartInit(UartDevice dev, u32 baud, u32 flags);
void uartWriteData(UartDevice dev, const void *buffer, size_t size);
void uartReadData(UartDevice dev, void *buffer, size_t size);
size_t uartReadDataMax(UartDevice dev, void *buffer, size_t maxSize);
ReadWriteDirection uartGetInterruptDirection(UartDevice dev);
void uartSetInterruptStatus(UartDevice dev, ReadWriteDirection direction, bool enable);

static inline u16 uartGetIrqId(UartDevice dev)
{
    static const u16 irqIds[] = { GIC_IRQID_UARTA, GIC_IRQID_UARTB, GIC_IRQID_UARTC, GIC_IRQID_UARTD };
    return dev < UART_MAX ? irqIds[dev] : GIC_IRQID_SPURIOUS;
}
