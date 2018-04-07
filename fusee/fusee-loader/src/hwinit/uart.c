#include "uart.h"
#include "t210.h"
#include "util.h"

/* UART A, B, C, D and E. */
static const u32 uart_baseoff[5] = { 0, 0x40, 0x200, 0x300, 0x400 };

void uart_init(u32 idx, u32 baud)
{
	volatile uart_t *uart = (uart_t *)(UART_BASE + uart_baseoff[idx]);

	//Set baud rate.
	u32 rate = (8 * baud + 408000000) / (16 * baud);
	uart->UART_LCR = 0x80; //Enable DLAB.
	uart->UART_THR_DLAB = (u8)rate; //Divisor latch LSB.
	uart->UART_IER_DLAB = (u8)(rate >> 8); //Divisor latch MSB.
	uart->UART_LCR = 0; //Diable DLAB.

	//Setup UART in fifo mode.
	uart->UART_IER_DLAB = 0;
	uart->UART_IIR_FCR = 7; //Enable and clear TX and RX FIFOs.
	volatile u32 tmp = uart->UART_LSR;
	sleep(3 * ((baud + 999999) / baud));
	uart->UART_LCR = 3; //Set word length 8.
	uart->UART_MCR = 0;
	uart->UART_MSR = 0;
	uart->UART_IRDA_CSR = 0;
	uart->UART_RX_FIFO_CFG = 1;
	uart->UART_MIE = 0;
	uart->UART_ASR = 0;
}

void uart_wait_idle(u32 idx, u32 which)
{
	volatile uart_t *uart = (uart_t *)(UART_BASE + uart_baseoff[idx]);
	while (!(uart->UART_VENDOR_STATUS & which))
		;
}

void uart_send(u32 idx, u8 *buf, u32 len)
{
	volatile uart_t *uart = (uart_t *)(UART_BASE + uart_baseoff[idx]);

	for (u32 i = 0; i != len; i++)
	{
		while (uart->UART_LSR & UART_TX_FIFO_FULL)
			;
		uart->UART_THR_DLAB = buf[i];
	};
}

void uart_recv(u32 idx, u8 *buf, u32 len)
{
	volatile uart_t *uart = (uart_t *)(UART_BASE + uart_baseoff[idx]);

	for (u32 i = 0; i != len; i++)
	{
		while (uart->UART_LSR & UART_RX_FIFO_EMPTY)
			;
		buf[i] = uart->UART_THR_DLAB;
	};
}
