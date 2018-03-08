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

/* Exosphère: add the clkreset values for UART C,D,E */
typedef enum {
    UART_A = 0,
    UART_B = 1,
    UART_C = 2,
    UART_D = 3,
    UART_E = 4,
} UartDevice;

#define BAUD_115200 115200

#define UART_TX_IDLE        0x00000001
#define UART_RX_IDLE        0x00000002
#define UART_TX_FIFO_FULL   0x100
#define UART_RX_FIFO_EMPTY  0x200

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
void uart_wait_idle(UartDevice dev, uint32_t which);
void uart_send(UartDevice dev, const void *buf, size_t len);
void uart_recv(UartDevice dev, void *buf, size_t len);

static inline volatile uart_t *get_uart_device(UartDevice dev) {
    static const size_t offsets[] = {0, 0x40, 0x200, 0x300, 0x400};
    return (volatile uart_t *)(UART_BASE + offsets[dev]);
}

#endif
