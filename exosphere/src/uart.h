#ifndef EXOSPHERE_UART_H
#define EXOSPHERE_UART_H

#include <stdint.h>
#include "memory_map.h"

/* Exosphere driver for the Tegra X1 UARTs. */
/* Mostly copied from https://github.com/nwert/hekate/blob/master/hwinit/uart.h and https://github.com/nwert/hekate/blob/master/hwinit/uart.c */

static inline uintptr_t get_uart_base(void) {
    return MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_UART);
}

#define UART_BASE (get_uart_base())

#define BAUD_115200 115200

/* Exosphère: add the clkreset values for UART C,D,E */
typedef enum {
    UART_A = 0,
    UART_B = 1,
    UART_C = 2,
    UART_D = 3,
    UART_E = 4,
} UartDevice;

typedef enum {
    UART_TX_IDLE = 1 << 0,
    UART_RX_IDLE = 1 << 1,

    /* This bit is set to 1 when a read is issued to an empty FIFO and gets cleared on register read (sticky bit until read)
    0 = NO_UNDERRUN
    1 = UNDERRUN */
    UART_UNDERRUN = 1 << 2,

    /* This bit is set to 1 when write data is issued to the TX FIFO when it is already full and gets cleared on register read (sticky bit until read)
    0 = NO_OVERRUN
    1 = OVERRUN */
    UART_OVERRUN = 1 << 3,

    RX_FIFO_COUNTER = 0b111111 << 16, /* reflects number of current entries in RX FIFO */
    TX_FIFO_COUNTER = 0b111111 << 24 /* reflects number of current entries in TX FIFO */
} UartVendorStatus;

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

typedef enum {
    UART_LCR_WD_LENGTH_5 = 0, /* word length 5 */
    UART_LCR_WD_LENGTH_6 = 1, /* word length 6 */
    UART_LCR_WD_LENGTH_7 = 2, /* word length 7 */
    UART_LCR_WD_LENGTH_8 = 3, /* word length 8 */

    /* STOP:
    0 = Transmit 1 stop bit
    1 = Transmit 2 stop bits (receiver always checks for 1 stop bit) */
    UART_LCR_STOP        = 1 << 2, 
    UART_LCR_PAR         = 1 << 3, /* Parity enabled */
    UART_LCR_EVEN        = 1 << 4, /* Even parity format. There will always be an even number of 1s in the binary representation (PAR = 1) */
    UART_LCR_SET_P       = 1 << 5, /* Set (force) parity to value in LCR[4] */
    UART_LCR_SET_B       = 1 << 6, /* Set BREAK condition -- Transmitter sends all zeroes to indicate BREAK */
    UART_LCR_DLAB        = 1 << 7, /* Divisor Latch Access Bit (set to allow programming of the DLH, DLM Divisors) */
} UartLineControl;

typedef struct {
    /* 0x00 */ uint32_t UART_THR_DLAB;
    /* 0x04 */ uint32_t UART_IER_DLAB;
    /* 0x08 */ uint32_t UART_IIR_FCR;
    /* 0x0C */ uint32_t UART_LCR;
    /* 0x10 */ uint32_t UART_MCR;
    /* 0x14 */ uint32_t UART_LSR;
    /* 0x18 */ uint32_t UART_MSR;
    /* 0x1C */ uint32_t UART_SPR;
    /* 0x20 */ uint32_t UART_IRDA_CSR;
    /* 0x24 */ uint32_t UART_RX_FIFO_CFG;
    /* 0x28 */ uint32_t UART_MIE;
    /* 0x2C */ uint32_t UART_VENDOR_STATUS;
    /* 0x30 */ uint8_t _pad_30[0x0C];
    /* 0x3C */ uint32_t UART_ASR;
} uart_t;

void uart_select(UartDevice dev);
void uart_init(UartDevice dev, uint32_t baud);
void uart_wait_idle(UartDevice dev, UartVendorStatus status);
void uart_send(UartDevice dev, const void *buf, size_t len);
void uart_recv(UartDevice dev, void *buf, size_t len);

static inline volatile uart_t *get_uart_device(UartDevice dev) {
    static const size_t offsets[] = {0, 0x40, 0x200, 0x300, 0x400};
    return (volatile uart_t *)(UART_BASE + offsets[dev]);
}

#endif
