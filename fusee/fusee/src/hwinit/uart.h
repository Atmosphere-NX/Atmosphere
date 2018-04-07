#ifndef _UART_H_
#define _UART_H_

#include "types.h"

#define UART_A 0
#define UART_B 1
#define UART_C 2
//TODO: define clock inits for those.
/*#define UART_D 3
#define UART_E 4*/

#define BAUD_115200 115200

#define UART_TX_IDLE 0x00000001
#define UART_RX_IDLE 0x00000002
#define UART_TX_FIFO_FULL 0x100
#define UART_RX_FIFO_EMPTY 0x200

typedef struct _uart_t
{
	/* 0x00 */ u32 UART_THR_DLAB;
	/* 0x04 */ u32 UART_IER_DLAB;
	/* 0x08 */ u32 UART_IIR_FCR;
	/* 0x0C */ u32 UART_LCR;
	/* 0x10 */ u32 UART_MCR;
	/* 0x14 */ u32 UART_LSR;
	/* 0x18 */ u32 UART_MSR;
	/* 0x1C */ u32 UART_SPR;
	/* 0x20 */ u32 UART_IRDA_CSR;
	/* 0x24 */ u32 UART_RX_FIFO_CFG;
	/* 0x28 */ u32 UART_MIE;
	/* 0x2C */ u32 UART_VENDOR_STATUS;
	/* 0x30 */ u8 _pad_30[0x0C];
	/* 0x3C */ u32 UART_ASR;
} uart_t;

void uart_init(u32 idx, u32 baud);
void uart_wait_idle(u32 idx, u32 which);
void uart_send(u32 idx, u8 *buf, u32 len);
void uart_recv(u32 idx, u8 *buf, u32 len);

#endif
