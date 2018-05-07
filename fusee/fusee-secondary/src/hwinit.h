#ifndef FUSEE_HWINIT_H
#define FUSEE_HWINIT_H

/* Symbols from hwinit that we're using, but w/o importing macro definitions that may clash with ours */

#include "hwinit/types.h"
#include "hwinit/hwinit.h"
#include "hwinit/i2c.h"

#include <stdbool.h>

#define UART_A 0
#define UART_B 1
#define UART_C 2
#define BAUD_115200 115200

void uart_init(u32 idx, u32 baud);
void uart_wait_idle(u32 idx, u32 which);
void uart_send(u32 idx, u8 *buf, u32 len);
void uart_recv(u32 idx, u8 *buf, u32 len);

void display_init();
void display_end();

void clock_enable_fuse(u32 enable);

/*! Show one single color on the display. */
void display_color_screen(u32 color);

/*! Init display in full 1280x720 resolution (32bpp, line stride 768, framebuffer size = 1280*768*4 bytes). */
u32 *display_init_framebuffer(void *address);

/*! Enable or disable the backlight. Should only be called when the screen is completely set up, to avoid flickering. */
void display_enable_backlight(bool on);

void cluster_enable_cpu0(u64 entry, u32 ns_disable);

#endif
