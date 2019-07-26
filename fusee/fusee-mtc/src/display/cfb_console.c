/*
 * (C) Copyright 2002 ELTEC Elektronik AG
 * Frank Gottschling <fgottschling@eltec.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * cfb_console.c
 *
 * Color Framebuffer Console driver for 8/15/16/24/32 bits per pixel.
 *
 * At the moment only the 8x16 font is tested and the font fore- and
 * background color is limited to black/white/gray colors. The Linux
 * logo can be placed in the upper left corner and additional board
 * information strings (that normaly goes to serial port) can be drawed.
 *
 * The console driver can use the standard PC keyboard interface (i8042)
 * for character input. Character output goes to a memory mapped video
 * framebuffer with little or big-endian organisation.
 * With environment setting 'console=serial' the console i/o can be
 * forced to serial port.

 The driver uses graphic specific defines/parameters/functions:

 VIDEO_FB_LITTLE_ENDIAN	 - framebuffer organisation default: big endian
 VIDEO_HW_RECTFILL	 - graphic driver supports hardware rectangle fill
 VIDEO_HW_BITBLT	 - graphic driver supports hardware bit blt

 Console Parameters are set by config:

 CONFIG_VIDEO_VISIBLE_COLS   - x resolution
 CONFIG_VIDEO_VISIBLE_ROWS   - y resolution
 CONFIG_VIDEO_PIXEL_SIZE     - storage size in byte per pixel
 CONFIG_VIDEO_DATA_FORMAT    - graphical data format GDF

 CONFIG_CONSOLE_CURSOR	     - on/off drawing cursor is done with delay
			       loop in VIDEO_TSTC_FCT (i8042)
 CONFIG_SYS_CONSOLE_BLINK_COUNT     - value for delay loop - blink rate
 CONFIG_CONSOLE_TIME	     - display time/date in upper right corner,
			       needs CONFIG_CMD_DATE and CONFIG_CONSOLE_CURSOR
 CONFIG_VIDEO_LOGO	     - display Linux Logo in upper left corner
 CONFIG_VIDEO_BMP_LOGO	     - use bmp_logo instead of linux_logo
 CONFIG_CONSOLE_EXTRA_INFO   - display additional board information strings
			       that normaly goes to serial port. This define
			       requires a board specific function:
			       video_drawstring (VIDEO_INFO_X,
						 VIDEO_INFO_Y + i*VIDEO_FONT_HEIGHT,
						 info);
			       that fills a info buffer at i=row.
			       s.a: board/eltec/bab7xx.
CONFIG_VGA_AS_SINGLE_DEVICE  - If set the framebuffer device will be initialised
			       as an output only device. The Keyboard driver
			       will not be set-up. This may be used, if you
			       have none or more than one Keyboard devices
			       (USB Keyboard, AT Keyboard).

CONFIG_VIDEO_SW_CURSOR:	     - Draws a cursor after the last character. No
			       blinking is provided. Uses the macros CURSOR_SET
			       and CURSOR_OFF.
CONFIG_VIDEO_HW_CURSOR:	     - Uses the hardware cursor capability of the
			       graphic chip. Uses the macro CURSOR_SET.
			       ATTENTION: If booting an OS, the display driver
			       must disable the hardware register of the graphic
			       chip. Otherwise a blinking field is displayed
*/

#include "video_fb.h"
#include <string.h>


#if defined(CONFIG_VIDEO_FONT_SMALL)
#include "video_font_small.h"
#else
#include "video_font_large.h"
#endif

#if defined(CONFIG_CMD_DATE)
#include <rtc.h>
#endif

#if defined(CONFIG_CMD_BMP) || defined(CONFIG_SPLASH_SCREEN)
#include <watchdog.h>
#include <bmp_layout.h>

#ifdef CONFIG_SPLASH_SCREEN_ALIGN
#define BMP_ALIGN_CENTER	0x7FFF
#endif

#endif

/*****************************************************************************/
/* Cursor definition:							     */
/* CONFIG_CONSOLE_CURSOR:  Uses a timer function (see drivers/input/i8042.c) */
/*                         to let the cursor blink. Uses the macros	     */
/*                         CURSOR_OFF and CURSOR_ON.			     */
/* CONFIG_VIDEO_SW_CURSOR: Draws a cursor after the last character. No	     */
/*			   blinking is provided. Uses the macros CURSOR_SET  */
/*			   and CURSOR_OFF.				     */
/* CONFIG_VIDEO_HW_CURSOR: Uses the hardware cursor capability of the	     */
/*			   graphic chip. Uses the macro CURSOR_SET.	     */
/*			   ATTENTION: If booting an OS, the display driver   */
/*			   must disable the hardware register of the graphic */
/*			   chip. Otherwise a blinking field is displayed     */
/*****************************************************************************/
#if !defined(CONFIG_CONSOLE_CURSOR) && \
    !defined(CONFIG_VIDEO_SW_CURSOR) && \
    !defined(CONFIG_VIDEO_HW_CURSOR)
/* no Cursor defined */
#define CURSOR_ON
#define CURSOR_OFF
#define CURSOR_SET
#endif

#ifdef	CONFIG_CONSOLE_CURSOR
#ifdef	CURSOR_ON
#error	only one of CONFIG_CONSOLE_CURSOR,CONFIG_VIDEO_SW_CURSOR,CONFIG_VIDEO_HW_CURSOR can be defined
#endif
void	console_cursor (int state);
#define CURSOR_ON  console_cursor(1)
#define CURSOR_OFF console_cursor(0)
#define CURSOR_SET
#else
#ifdef	CONFIG_CONSOLE_TIME
#error	CONFIG_CONSOLE_CURSOR must be defined for CONFIG_CONSOLE_TIME
#endif
#endif /* CONFIG_CONSOLE_CURSOR */

#ifdef	CONFIG_VIDEO_SW_CURSOR
#ifdef	CURSOR_ON
#error	only one of CONFIG_CONSOLE_CURSOR,CONFIG_VIDEO_SW_CURSOR,CONFIG_VIDEO_HW_CURSOR can be defined
#endif
#define CURSOR_ON
#define CURSOR_OFF video_putchar(console_col * VIDEO_FONT_WIDTH,\
				 console_row * VIDEO_FONT_HEIGHT, ' ')
#define CURSOR_SET video_set_cursor()
#endif /* CONFIG_VIDEO_SW_CURSOR */


#ifdef CONFIG_VIDEO_HW_CURSOR
#ifdef	CURSOR_ON
#error	only one of CONFIG_CONSOLE_CURSOR,CONFIG_VIDEO_SW_CURSOR,CONFIG_VIDEO_HW_CURSOR can be defined
#endif
#define CURSOR_ON
#define CURSOR_OFF
#define CURSOR_SET video_set_hw_cursor(console_col * VIDEO_FONT_WIDTH, \
		  (console_row * VIDEO_FONT_HEIGHT) + video_logo_height)
#endif	/* CONFIG_VIDEO_HW_CURSOR */

#ifdef	CONFIG_VIDEO_LOGO
#ifdef	CONFIG_VIDEO_BMP_LOGO
#include <bmp_logo.h>
#define VIDEO_LOGO_WIDTH	BMP_LOGO_WIDTH
#define VIDEO_LOGO_HEIGHT	BMP_LOGO_HEIGHT
#define VIDEO_LOGO_LUT_OFFSET	BMP_LOGO_OFFSET
#define VIDEO_LOGO_COLORS	BMP_LOGO_COLORS

#else	/* CONFIG_VIDEO_BMP_LOGO */
#define LINUX_LOGO_WIDTH	80
#define LINUX_LOGO_HEIGHT	80
#define LINUX_LOGO_COLORS	214
#define LINUX_LOGO_LUT_OFFSET	0x20
#define __initdata
#include <linux_logo.h>
#define VIDEO_LOGO_WIDTH	LINUX_LOGO_WIDTH
#define VIDEO_LOGO_HEIGHT	LINUX_LOGO_HEIGHT
#define VIDEO_LOGO_LUT_OFFSET	LINUX_LOGO_LUT_OFFSET
#define VIDEO_LOGO_COLORS	LINUX_LOGO_COLORS
#endif	/* CONFIG_VIDEO_BMP_LOGO */
#define VIDEO_INFO_X		(VIDEO_LOGO_WIDTH)
#define VIDEO_INFO_Y		(VIDEO_FONT_HEIGHT/2)
#else	/* CONFIG_VIDEO_LOGO */
#define VIDEO_LOGO_WIDTH	0
#define VIDEO_LOGO_HEIGHT	0
#endif	/* CONFIG_VIDEO_LOGO */

#ifdef CONFIG_VIDEO_COLS
#define VIDEO_COLS		CONFIG_VIDEO_COLS
#else
#define VIDEO_COLS		CONFIG_VIDEO_VISIBLE_COLS
#endif

#define VIDEO_ROWS		CONFIG_VIDEO_VISIBLE_ROWS
#define VIDEO_SIZE		(VIDEO_ROWS*VIDEO_COLS*CONFIG_VIDEO_PIXEL_SIZE)
#define VIDEO_PIX_BLOCKS	(VIDEO_SIZE >> 2)
#define VIDEO_LINE_LEN		(VIDEO_COLS*CONFIG_VIDEO_PIXEL_SIZE)
#define VIDEO_BURST_LEN		(VIDEO_COLS/8)

#ifdef	CONFIG_VIDEO_LOGO
#define CONSOLE_ROWS		((VIDEO_ROWS - video_logo_height) / VIDEO_FONT_HEIGHT)
#else
#define CONSOLE_ROWS		(VIDEO_ROWS / VIDEO_FONT_HEIGHT)
#endif

#define CONSOLE_COLS		(VIDEO_COLS / VIDEO_FONT_WIDTH)
#define CONSOLE_ROW_SIZE	(VIDEO_FONT_HEIGHT * VIDEO_LINE_LEN)
#define CONSOLE_ROW_FIRST	(video_console_address)
#define CONSOLE_ROW_SECOND	(video_console_address + CONSOLE_ROW_SIZE)
#define CONSOLE_ROW_LAST	(video_console_address + CONSOLE_SIZE - CONSOLE_ROW_SIZE)
#define CONSOLE_SIZE		(CONSOLE_ROW_SIZE * CONSOLE_ROWS)
#define CONSOLE_SCROLL_SIZE	(CONSOLE_SIZE - CONSOLE_ROW_SIZE)

/* Macros */
#ifdef	VIDEO_FB_LITTLE_ENDIAN
#define SWAP16(x)	 ((((x) & 0x00ff) << 8) | ( (x) >> 8))
#define SWAP32(x)	 ((((x) & 0x000000ff) << 24) | (((x) & 0x0000ff00) << 8)|\
			  (((x) & 0x00ff0000) >>  8) | (((x) & 0xff000000) >> 24) )
#define SHORTSWAP32(x)	 ((((x) & 0x000000ff) <<  8) | (((x) & 0x0000ff00) >> 8)|\
			  (((x) & 0x00ff0000) <<  8) | (((x) & 0xff000000) >> 8) )
#else
#define SWAP16(x)	 (x)
#define SWAP32(x)	 (x)
#if defined(VIDEO_FB_16BPP_WORD_SWAP)
#define SHORTSWAP32(x)	 ( ((x) >> 16) | ((x) << 16) )
#else
#define SHORTSWAP32(x)	 (x)
#endif
#endif

#if defined(DEBUG) || defined(DEBUG_CFB_CONSOLE)
#define PRINTD(x)	  printf(x)
#else
#define PRINTD(x)
#endif


#ifdef CONFIG_CONSOLE_EXTRA_INFO
extern void video_get_info_str (    /* setup a board string: type, speed, etc. */
    int line_number,	    /* location to place info string beside logo */
    char *info		    /* buffer for info string */
    );

#endif

#define ALIGN_UP(addr, align) (((addr) + (align) - 1) & (~((align) - 1)))
#define WIDTH_BYTES (ALIGN_UP(VIDEO_FONT_WIDTH, 8) / 8)

static void *video_fb_address;		/* frame buffer address */
static void *video_console_address;	/* console buffer start address */

static int video_logo_height = VIDEO_LOGO_HEIGHT;

static int console_col = 0; /* cursor col */
static int console_row = 0; /* cursor row */

static uint32_t eorx, fgx, bgx;  /* color pats */

static const int video_font_draw_table8[] = {
	    0x00000000, 0x000000ff, 0x0000ff00, 0x0000ffff,
	    0x00ff0000, 0x00ff00ff, 0x00ffff00, 0x00ffffff,
	    0xff000000, 0xff0000ff, 0xff00ff00, 0xff00ffff,
	    0xffff0000, 0xffff00ff, 0xffffff00, 0xffffffff };

static const int video_font_draw_table15[] = {
	    0x00000000, 0x00007fff, 0x7fff0000, 0x7fff7fff };

static const int video_font_draw_table16[] = {
	    0x00000000, 0x0000ffff, 0xffff0000, 0xffffffff };

static const int video_font_draw_table24[16][3] = {
	    { 0x00000000, 0x00000000, 0x00000000 },
	    { 0x00000000, 0x00000000, 0x00ffffff },
	    { 0x00000000, 0x0000ffff, 0xff000000 },
	    { 0x00000000, 0x0000ffff, 0xffffffff },
	    { 0x000000ff, 0xffff0000, 0x00000000 },
	    { 0x000000ff, 0xffff0000, 0x00ffffff },
	    { 0x000000ff, 0xffffffff, 0xff000000 },
	    { 0x000000ff, 0xffffffff, 0xffffffff },
	    { 0xffffff00, 0x00000000, 0x00000000 },
	    { 0xffffff00, 0x00000000, 0x00ffffff },
	    { 0xffffff00, 0x0000ffff, 0xff000000 },
	    { 0xffffff00, 0x0000ffff, 0xffffffff },
	    { 0xffffffff, 0xffff0000, 0x00000000 },
	    { 0xffffffff, 0xffff0000, 0x00ffffff },
	    { 0xffffffff, 0xffffffff, 0xff000000 },
	    { 0xffffffff, 0xffffffff, 0xffffffff } };

static const int video_font_draw_table32[16][4] = {
	    { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
	    { 0x00000000, 0x00000000, 0x00000000, 0x00ffffff },
	    { 0x00000000, 0x00000000, 0x00ffffff, 0x00000000 },
	    { 0x00000000, 0x00000000, 0x00ffffff, 0x00ffffff },
	    { 0x00000000, 0x00ffffff, 0x00000000, 0x00000000 },
	    { 0x00000000, 0x00ffffff, 0x00000000, 0x00ffffff },
	    { 0x00000000, 0x00ffffff, 0x00ffffff, 0x00000000 },
	    { 0x00000000, 0x00ffffff, 0x00ffffff, 0x00ffffff },
	    { 0x00ffffff, 0x00000000, 0x00000000, 0x00000000 },
	    { 0x00ffffff, 0x00000000, 0x00000000, 0x00ffffff },
	    { 0x00ffffff, 0x00000000, 0x00ffffff, 0x00000000 },
	    { 0x00ffffff, 0x00000000, 0x00ffffff, 0x00ffffff },
	    { 0x00ffffff, 0x00ffffff, 0x00000000, 0x00000000 },
	    { 0x00ffffff, 0x00ffffff, 0x00000000, 0x00ffffff },
	    { 0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00000000 },
	    { 0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff } };


/******************************************************************************/

static void video_drawchars (int xx, int yy, unsigned char *s, int count)
{
	uint8_t *cdat, *dest, *dest0;
	int rows, offset, c, cols, tbits;

	offset = yy * VIDEO_LINE_LEN + xx * CONFIG_VIDEO_PIXEL_SIZE;
	dest0 = video_fb_address + offset;

	switch (CONFIG_VIDEO_DATA_FORMAT) {
	case GDF__8BIT_INDEX:
	case GDF__8BIT_332RGB:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * (VIDEO_FONT_HEIGHT * WIDTH_BYTES);
			for (rows = VIDEO_FONT_HEIGHT, dest = dest0;
			     rows--;
			     dest += VIDEO_LINE_LEN) {
				cols = 0;
				tbits = VIDEO_FONT_WIDTH;
				while (tbits > 0) {
					uint8_t bits = *cdat++;

					((uint32_t *) dest)[cols + 0] = (video_font_draw_table8[bits >> 4] & eorx) ^ bgx;
					((uint32_t *) dest)[cols + 1] = (video_font_draw_table8[bits & 15] & eorx) ^ bgx;
					cols += 8;
					tbits -= 8;
				}
			}
			dest0 += VIDEO_FONT_WIDTH * CONFIG_VIDEO_PIXEL_SIZE;
			s++;
		}
		break;

	case GDF_15BIT_555RGB:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * (VIDEO_FONT_HEIGHT * WIDTH_BYTES);
			for (rows = VIDEO_FONT_HEIGHT, dest = dest0;
			     rows--;
			     dest += VIDEO_LINE_LEN) {
				cols = 0;
				tbits = VIDEO_FONT_WIDTH;
				while (tbits > 0) {
					uint8_t bits = *cdat++;

					((uint32_t *) dest)[cols + 0] = SHORTSWAP32 ((video_font_draw_table15 [bits >> 6] & eorx) ^ bgx);
					((uint32_t *) dest)[cols + 1] = SHORTSWAP32 ((video_font_draw_table15 [bits >> 4 & 3] & eorx) ^ bgx);
					((uint32_t *) dest)[cols + 2] = SHORTSWAP32 ((video_font_draw_table15 [bits >> 2 & 3] & eorx) ^ bgx);
					((uint32_t *) dest)[cols + 3] = SHORTSWAP32 ((video_font_draw_table15 [bits & 3] & eorx) ^ bgx);
					cols += 8;
					tbits -= 8;
				}
			}
			dest0 += VIDEO_FONT_WIDTH * CONFIG_VIDEO_PIXEL_SIZE;
			s++;
		}
		break;

	case GDF_16BIT_565RGB:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * (VIDEO_FONT_HEIGHT * WIDTH_BYTES);
			for (rows = VIDEO_FONT_HEIGHT, dest = dest0;
			     rows--;
			     dest += VIDEO_LINE_LEN) {
				cols = 0;
				tbits = VIDEO_FONT_WIDTH;
				while (tbits > 0) {
					uint8_t bits = *cdat++;

					((uint32_t *) dest)[cols + 0] = SHORTSWAP32 ((video_font_draw_table16 [bits >> 6] & eorx) ^ bgx);
					((uint32_t *) dest)[cols + 1] = SHORTSWAP32 ((video_font_draw_table16 [bits >> 4 & 3] & eorx) ^ bgx);
					((uint32_t *) dest)[cols + 2] = SHORTSWAP32 ((video_font_draw_table16 [bits >> 2 & 3] & eorx) ^ bgx);
					((uint32_t *) dest)[cols + 3] = SHORTSWAP32 ((video_font_draw_table16 [bits & 3] & eorx) ^ bgx);
					cols += 8;
					tbits -= 8;
				}
			}
			dest0 += VIDEO_FONT_WIDTH * CONFIG_VIDEO_PIXEL_SIZE;
			s++;
		}
		break;

	case GDF_32BIT_X888RGB:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * (VIDEO_FONT_HEIGHT * WIDTH_BYTES);
			for (rows = VIDEO_FONT_HEIGHT, dest = dest0;
			     rows--;
			     dest += VIDEO_LINE_LEN) {
				cols = 0;
				tbits = VIDEO_FONT_WIDTH;
				while (tbits > 0) {
					uint8_t bits = *cdat++;

					((uint32_t *) dest)[cols + 0] = SWAP32 ((video_font_draw_table32 [bits >> 4][0] & eorx) ^ bgx);
					((uint32_t *) dest)[cols + 1] = SWAP32 ((video_font_draw_table32 [bits >> 4][1] & eorx) ^ bgx);
					((uint32_t *) dest)[cols + 2] = SWAP32 ((video_font_draw_table32 [bits >> 4][2] & eorx) ^ bgx);
					((uint32_t *) dest)[cols + 3] = SWAP32 ((video_font_draw_table32 [bits >> 4][3] & eorx) ^ bgx);
					((uint32_t *) dest)[cols + 4] = SWAP32 ((video_font_draw_table32 [bits & 15][0] & eorx) ^ bgx);
					((uint32_t *) dest)[cols + 5] = SWAP32 ((video_font_draw_table32 [bits & 15][1] & eorx) ^ bgx);
					((uint32_t *) dest)[cols + 6] = SWAP32 ((video_font_draw_table32 [bits & 15][2] & eorx) ^ bgx);
					((uint32_t *) dest)[cols + 7] = SWAP32 ((video_font_draw_table32 [bits & 15][3] & eorx) ^ bgx);
					cols += 8;
					tbits -= 8;
				}
			}
			dest0 += VIDEO_FONT_WIDTH * CONFIG_VIDEO_PIXEL_SIZE;
			s++;
		}
		break;

	case GDF_24BIT_888RGB:
		while (count--) {
			c = *s;
			cdat = video_fontdata + c * (VIDEO_FONT_HEIGHT * WIDTH_BYTES);
			for (rows = VIDEO_FONT_HEIGHT, dest = dest0;
			     rows--;
			     dest += VIDEO_LINE_LEN) {
				cols = 0;
				tbits = VIDEO_FONT_WIDTH;
				while (tbits > 0) {
					uint8_t bits = *cdat++;

					((uint32_t *) dest)[cols + 0] = (video_font_draw_table24[bits >> 4][0] & eorx) ^ bgx;
					((uint32_t *) dest)[cols + 1] = (video_font_draw_table24[bits >> 4][1] & eorx) ^ bgx;
					((uint32_t *) dest)[cols + 2] = (video_font_draw_table24[bits >> 4][2] & eorx) ^ bgx;
					((uint32_t *) dest)[cols + 3] = (video_font_draw_table24[bits & 15][0] & eorx) ^ bgx;
					((uint32_t *) dest)[cols + 4] = (video_font_draw_table24[bits & 15][1] & eorx) ^ bgx;
					((uint32_t *) dest)[cols + 5] = (video_font_draw_table24[bits & 15][2] & eorx) ^ bgx;
					cols += 8;
					tbits -= 8;
				}
			}
			dest0 += VIDEO_FONT_WIDTH * CONFIG_VIDEO_PIXEL_SIZE;
			s++;
		}
		break;
	}
}

/*****************************************************************************/

static inline void video_drawstring (int xx, int yy, unsigned char *s)
{
	video_drawchars (xx, yy, s, strlen ((char *)s));
}

/*****************************************************************************/

static void video_putchar (int xx, int yy, unsigned char c)
{
	video_drawchars (xx, yy + video_logo_height, &c, 1);
}

/*****************************************************************************/
#if defined(CONFIG_CONSOLE_CURSOR) || defined(CONFIG_VIDEO_SW_CURSOR)
static void video_set_cursor (void)
{
	/* swap drawing colors */
	eorx = fgx;
	fgx = bgx;
	bgx = eorx;
	eorx = fgx ^ bgx;
	/* draw cursor */
	video_putchar (console_col * VIDEO_FONT_WIDTH,
		       console_row * VIDEO_FONT_HEIGHT,
		       ' ');
	/* restore drawing colors */
	eorx = fgx;
	fgx = bgx;
	bgx = eorx;
	eorx = fgx ^ bgx;
}
#endif
/*****************************************************************************/
#ifdef CONFIG_CONSOLE_CURSOR
void console_cursor (int state)
{
	static int last_state = 0;

#ifdef CONFIG_CONSOLE_TIME
	struct rtc_time tm;
	char info[16];

	/* time update only if cursor is on (faster scroll) */
	if (state) {
		rtc_get (&tm);

		sprintf (info, " %02d:%02d:%02d ", tm.tm_hour, tm.tm_min,
			 tm.tm_sec);
		video_drawstring (CONFIG_VIDEO_VISIBLE_COLS - 10 * VIDEO_FONT_WIDTH,
				  VIDEO_INFO_Y, (uchar *)info);

		sprintf (info, "%02d.%02d.%04d", tm.tm_mday, tm.tm_mon,
			 tm.tm_year);
		video_drawstring (CONFIG_VIDEO_VISIBLE_COLS - 10 * VIDEO_FONT_WIDTH,
				  VIDEO_INFO_Y + 1 * VIDEO_FONT_HEIGHT, (uchar *)info);
	}
#endif

	if (state && (last_state != state)) {
		video_set_cursor ();
	}

	if (!state && (last_state != state)) {
		/* clear cursor */
		video_putchar (console_col * VIDEO_FONT_WIDTH,
			       console_row * VIDEO_FONT_HEIGHT,
			       ' ');
	}

	last_state = state;
}
#endif

/*****************************************************************************/

#ifndef VIDEO_HW_RECTFILL
static void memsetl (int *p, int c, int v)
{
	while (c--)
		*(p++) = v;
}
#endif

/*****************************************************************************/


static void console_scrollup (void)
{
	/* copy up rows ignoring the first one */

#ifdef VIDEO_HW_BITBLT
	video_hw_bitblt (CONFIG_VIDEO_PIXEL_SIZE,	/* bytes per pixel */
			 0,	/* source pos x */
			 video_logo_height + VIDEO_FONT_HEIGHT, /* source pos y */
			 0,	/* dest pos x */
			 video_logo_height,	/* dest pos y */
			 CONFIG_VIDEO_VISIBLE_COLS,	/* frame width */
			 CONFIG_VIDEO_VISIBLE_ROWS - video_logo_height - VIDEO_FONT_HEIGHT	/* frame height */
		);
#else
	memmove(CONSOLE_ROW_FIRST, CONSOLE_ROW_SECOND,
	       CONSOLE_SCROLL_SIZE);
#endif

	/* clear the last one */
#ifdef VIDEO_HW_RECTFILL
	video_hw_rectfill (CONFIG_VIDEO_PIXEL_SIZE,	/* bytes per pixel */
			   0,	/* dest pos x */
			   CONFIG_VIDEO_VISIBLE_ROWS - VIDEO_FONT_HEIGHT,	/* dest pos y */
			   CONFIG_VIDEO_VISIBLE_COLS,	/* frame width */
			   VIDEO_FONT_HEIGHT,	/* frame height */
			   CONSOLE_BG_COL	/* fill color */
		);
#else
	memsetl (CONSOLE_ROW_LAST, CONSOLE_ROW_SIZE >> 2, CONSOLE_BG_COL);
#endif
}

/*****************************************************************************/

static void console_back (void)
{
	CURSOR_OFF;
	console_col--;

	if (console_col < 0) {
		console_col = CONSOLE_COLS - 1;
		console_row--;
		if (console_row < 0)
			console_row = 0;
	}
	video_putchar (console_col * VIDEO_FONT_WIDTH,
		       console_row * VIDEO_FONT_HEIGHT,
		       ' ');
}

/*****************************************************************************/

static void console_newline (void)
{
	/* Check if last character in the line was just drawn. If so, cursor was
	   overwriten and need not to be cleared. Cursor clearing without this
	   check causes overwriting the 1st character of the line if line lenght
	   is >= CONSOLE_COLS
	 */
	if (console_col < CONSOLE_COLS)
		CURSOR_OFF;
	console_row++;
	console_col = 0;

	/* Check if we need to scroll the terminal */
	if (console_row >= CONSOLE_ROWS) {
		/* Scroll everything up */
		console_scrollup ();

		/* Decrement row number */
		console_row--;
	}
}

static void console_cr (void)
{
	CURSOR_OFF;
	console_col = 0;
}

/*****************************************************************************/

void video_putc (const char c)
{
	static int nl = 1;

	switch (c) {
	case 13:		/* back to first column */
		console_cr ();
		break;

	case '\n':		/* next line */
		if (console_col || (!console_col && nl))
			console_newline ();
		nl = 1;
		break;

	case 9:		/* tab 8 */
		CURSOR_OFF;
		console_col |= 0x0008;
		console_col &= ~0x0007;

		if (console_col >= CONSOLE_COLS)
			console_newline ();
		break;

	case 8:		/* backspace */
		console_back ();
		break;

	default:		/* draw the char */
		video_putchar (console_col * VIDEO_FONT_WIDTH,
			       console_row * VIDEO_FONT_HEIGHT,
			       c);
		console_col++;

		/* check for newline */
		if (console_col >= CONSOLE_COLS) {
			console_newline ();
			nl = 0;
		}
	}
	CURSOR_SET;
}


/*****************************************************************************/

void video_puts (const char *s)
{
	int count = strlen (s);

	while (count--)
		video_putc (*s++);
}

/*****************************************************************************/

/*
 * Do not enforce drivers (or board code) to provide empty
 * video_set_lut() if they do not support 8 bpp format.
 * Implement weak default function instead.
 */
void __video_set_lut (unsigned int index, unsigned char r,
		      unsigned char g, unsigned char b)
{
}
void video_set_lut (unsigned int, unsigned char, unsigned char, unsigned char)
			__attribute__((weak, alias("__video_set_lut")));

#if defined(CONFIG_CMD_BMP) || defined(CONFIG_SPLASH_SCREEN)

#define FILL_8BIT_332RGB(r,g,b)	{			\
	*fb = ((r>>5)<<5) | ((g>>5)<<2) | (b>>6);	\
	fb ++;						\
}

#define FILL_15BIT_555RGB(r,g,b) {			\
	*(unsigned short *)fb = SWAP16((unsigned short)(((r>>3)<<10) | ((g>>3)<<5) | (b>>3))); \
	fb += 2;					\
}

#define FILL_16BIT_565RGB(r,g,b) {			\
	*(unsigned short *)fb = SWAP16((unsigned short)((((r)>>3)<<11) | (((g)>>2)<<5) | ((b)>>3))); \
	fb += 2;					\
}

#define FILL_32BIT_X888RGB(r,g,b) {			\
	*(unsigned long *)fb = SWAP32((unsigned long)(((r<<16) | (g<<8) | b))); \
	fb += 4;					\
}

#ifdef VIDEO_FB_LITTLE_ENDIAN
#define FILL_24BIT_888RGB(r,g,b) {			\
	fb[0] = b;					\
	fb[1] = g;					\
	fb[2] = r;					\
	fb += 3;					\
}
#else
#define FILL_24BIT_888RGB(r,g,b) {			\
	fb[0] = r;					\
	fb[1] = g;					\
	fb[2] = b;					\
	fb += 3;					\
}
#endif

#if defined(VIDEO_FB_16BPP_PIXEL_SWAP)
static void inline fill_555rgb_pswap(uchar *fb, int x,
				     uint8_t r, uint8_t g, uint8_t b)
{
	ushort *dst = (ushort *)fb;
	ushort color = (ushort)(((r >> 3) << 10) |
				((g >> 3) << 5) |
				(b >> 3));
	if (x & 1)
		*(--dst) = color;
	else
		*(++dst) = color;
}
#endif

/*
 * RLE8 bitmap support
 */

#ifdef CONFIG_VIDEO_BMP_RLE8
/* Pre-calculated color table entry */
struct palette {
	union {
		unsigned short	w;	/* word */
		unsigned int	dw;	/* double word */
	} ce; /* color entry */
};

/*
 * Helper to draw encoded/unencoded run.
 */
static void draw_bitmap (uchar **fb, uchar *bm, struct palette *p,
			 int cnt, int enc)
{
	ulong addr = (ulong)*fb;
	int *off;
	int enc_off = 1;
	int i;

	/*
	 * Setup offset of the color index in the bitmap.
	 * Color index of encoded run is at offset 1.
	 */
	off = enc ? &enc_off : &i;

	switch (CONFIG_VIDEO_DATA_FORMAT) {
	case GDF__8BIT_INDEX:
		for (i = 0; i < cnt; i++)
			*(unsigned char *)addr++ = bm[*off];
		break;
	case GDF_15BIT_555RGB:
	case GDF_16BIT_565RGB:
		/* differences handled while pre-calculating palette */
		for (i = 0; i < cnt; i++) {
			*(unsigned short *)addr = p[bm[*off]].ce.w;
			addr += 2;
		}
		break;
	case GDF_32BIT_X888RGB:
		for (i = 0; i < cnt; i++) {
			*(unsigned long *)addr = p[bm[*off]].ce.dw;
			addr += 4;
		}
		break;
	}
	*fb = (uchar *)addr; /* return modified address */
}

static int display_rle8_bitmap (bmp_image_t *img, int xoff, int yoff,
				int width, int height)
{
	unsigned char *bm;
	unsigned char *fbp;
	unsigned int cnt, runlen;
	int decode = 1;
	int x, y, bpp, i, ncolors;
	struct palette p[256];
	bmp_color_table_entry_t cte;
	int green_shift, red_off;

	x = 0;
	y = __le32_to_cpu(img->header.height) - 1;
	ncolors = __le32_to_cpu(img->header.colors_used);
	bpp = CONFIG_VIDEO_PIXEL_SIZE;
	fbp = (unsigned char *)((unsigned int)video_fb_address +
				(((y + yoff) * VIDEO_COLS) + xoff) * bpp);

	bm = (uchar *)img + __le32_to_cpu(img->header.data_offset);

	/* pre-calculate and setup palette */
	switch (CONFIG_VIDEO_DATA_FORMAT) {
	case GDF__8BIT_INDEX:
		for (i = 0; i < ncolors; i++) {
			cte = img->color_table[i];
			video_set_lut (i, cte.red, cte.green, cte.blue);
		}
		break;
	case GDF_15BIT_555RGB:
	case GDF_16BIT_565RGB:
		if (CONFIG_VIDEO_DATA_FORMAT == GDF_15BIT_555RGB) {
			green_shift = 3;
			red_off = 10;
		} else {
			green_shift = 2;
			red_off = 11;
		}
		for (i = 0; i < ncolors; i++) {
			cte = img->color_table[i];
			p[i].ce.w = SWAP16((unsigned short)
					   (((cte.red >> 3) << red_off) |
					    ((cte.green >> green_shift) << 5) |
					    cte.blue >> 3));
		}
		break;
	case GDF_32BIT_X888RGB:
		for (i = 0; i < ncolors; i++) {
			cte = img->color_table[i];
			p[i].ce.dw = SWAP32((cte.red << 16) | (cte.green << 8) |
					     cte.blue);
		}
		break;
	default:
		printf("RLE Bitmap unsupported in video mode 0x%x\n",
		       CONFIG_VIDEO_DATA_FORMAT);
		return -1;
	}

	while (decode) {
		switch (bm[0]) {
		case 0:
			switch (bm[1]) {
			case 0:
				/* scan line end marker */
				bm += 2;
				x = 0;
				y--;
				fbp = (unsigned char *)
					((unsigned int)video_fb_address +
					 (((y + yoff) * VIDEO_COLS) +
					  xoff) * bpp);
				continue;
			case 1:
				/* end of bitmap data marker */
				decode = 0;
				break;
			case 2:
				/* run offset marker */
				x += bm[2];
				y -= bm[3];
				fbp = (unsigned char *)
					((unsigned int)video_fb_address +
					 (((y + yoff) * VIDEO_COLS) +
					  x + xoff) * bpp);
				bm += 4;
				break;
			default:
				/* unencoded run */
				cnt = bm[1];
				runlen = cnt;
				bm += 2;
				if (y < height) {
					if (x >= width) {
						x += runlen;
						goto next_run;
					}
					if (x + runlen > width)
						cnt = width - x;

					draw_bitmap (&fbp, bm, p, cnt, 0);
					x += runlen;
				}
next_run:
				bm += runlen;
				if (runlen & 1)
					bm++; /* 0 padding if length is odd */
			}
			break;
		default:
			/* encoded run */
			if (y < height) { /* only draw into visible area */
				cnt = bm[0];
				runlen = cnt;
				if (x >= width) {
					x += runlen;
					bm += 2;
					continue;
				}
				if (x + runlen > width)
					cnt = width - x;

				draw_bitmap (&fbp, bm, p, cnt, 1);
				x += runlen;
			}
			bm += 2;
			break;
		}
	}
	return 0;
}
#endif

/*
 * Display the BMP file located at address bmp_image.
 */
int video_display_bitmap (ulong bmp_image, int x, int y)
{
	unsigned xcount, ycount;
	unsigned *fb;
	bmp_image_t *bmp = (bmp_image_t *) bmp_image;
	uchar *bmap;
	unsigned padded_line;
	unsigned long width, height, bpp;
	unsigned colors;
	unsigned long compression;
	bmp_color_table_entry_t cte;
#ifdef CONFIG_VIDEO_BMP_GZIP
	unsigned char *dst = NULL;
	ulong len;
#endif

	WATCHDOG_RESET ();

	if (!((bmp->header.signature[0] == 'B') &&
	      (bmp->header.signature[1] == 'M'))) {

#ifdef CONFIG_VIDEO_BMP_GZIP
		/*
		 * Could be a gzipped bmp image, try to decrompress...
		 */
		len = CONFIG_SYS_VIDEO_LOGO_MAX_SIZE;
		dst = malloc(CONFIG_SYS_VIDEO_LOGO_MAX_SIZE);
		if (dst == NULL) {
			printf("Error: malloc in gunzip failed!\n");
			return(1);
		}
		if (gunzip(dst, CONFIG_SYS_VIDEO_LOGO_MAX_SIZE, (uchar *)bmp_image, &len) != 0) {
			printf ("Error: no valid bmp or bmp.gz image at %lx\n", bmp_image);
			free(dst);
			return 1;
		}
		if (len == CONFIG_SYS_VIDEO_LOGO_MAX_SIZE) {
			printf("Image could be truncated (increase CONFIG_SYS_VIDEO_LOGO_MAX_SIZE)!\n");
		}

		/*
		 * Set addr to decompressed image
		 */
		bmp = (bmp_image_t *)dst;

		if (!((bmp->header.signature[0] == 'B') &&
		      (bmp->header.signature[1] == 'M'))) {
			printf ("Error: no valid bmp.gz image at %lx\n", bmp_image);
			free(dst);
			return 1;
		}
#else
		printf ("Error: no valid bmp image at %lx\n", bmp_image);
		return 1;
#endif /* CONFIG_VIDEO_BMP_GZIP */
	}

	width = le32_to_cpu (bmp->header.width);
	height = le32_to_cpu (bmp->header.height);
	bpp = le16_to_cpu (bmp->header.bit_count);
	colors = le32_to_cpu (bmp->header.colors_used);
	compression = le32_to_cpu (bmp->header.compression);

	debug ("Display-bmp: %d x %d  with %d colors\n",
	       width, height, colors);

	if (compression != BMP_BI_RGB
#ifdef CONFIG_VIDEO_BMP_RLE8
	    && compression != BMP_BI_RLE8
#endif
	   ) {
		printf ("Error: compression type %ld not supported\n",
			compression);
#ifdef CONFIG_VIDEO_BMP_GZIP
		if (dst)
			free(dst);
#endif
		return 1;
	}

	padded_line = (((width * bpp + 7) / 8) + 3) & ~0x3;

#ifdef CONFIG_SPLASH_SCREEN_ALIGN
	if (x == BMP_ALIGN_CENTER)
		x = max(0, (CONFIG_VIDEO_VISIBLE_COLS - width) / 2);
	else if (x < 0)
		x = max(0, CONFIG_VIDEO_VISIBLE_COLS - width + x + 1);

	if (y == BMP_ALIGN_CENTER)
		y = max(0, (CONFIG_VIDEO_VISIBLE_ROWS - height) / 2);
	else if (y < 0)
		y = max(0, CONFIG_VIDEO_VISIBLE_ROWS - height + y + 1);
#endif /* CONFIG_SPLASH_SCREEN_ALIGN */

	if ((x + width) > CONFIG_VIDEO_VISIBLE_COLS)
		width = CONFIG_VIDEO_VISIBLE_COLS - x;
	if ((y + height) > CONFIG_VIDEO_VISIBLE_ROWS)
		height = CONFIG_VIDEO_VISIBLE_ROWS - y;

	bmap = (uchar *) bmp + le32_to_cpu (bmp->header.data_offset);
	fb = (uchar *) (video_fb_address +
			((y + height - 1) * VIDEO_COLS * CONFIG_VIDEO_PIXEL_SIZE) +
			x * CONFIG_VIDEO_PIXEL_SIZE);

#ifdef CONFIG_VIDEO_BMP_RLE8
	if (compression == BMP_BI_RLE8) {
		return display_rle8_bitmap(bmp,
					   x, y, width, height);
	}
#endif

	/* We handle only 4, 8, or 24 bpp bitmaps */
	switch (le16_to_cpu (bmp->header.bit_count)) {
	case 4:
		padded_line -= width / 2;
		ycount = height;

		switch (CONFIG_VIDEO_DATA_FORMAT) {
		case GDF_32BIT_X888RGB:
			while (ycount--) {
				WATCHDOG_RESET ();
				/*
				 * Don't assume that 'width' is an
				 * even number
				 */
				for (xcount = 0; xcount < width; xcount++) {
					uchar idx;

					if (xcount & 1) {
						idx = *bmap & 0xF;
						bmap++;
					} else
						idx = *bmap >> 4;
					cte = bmp->color_table[idx];
					FILL_32BIT_X888RGB(cte.red, cte.green,
							   cte.blue);
				}
				bmap += padded_line;
				fb -= (CONFIG_VIDEO_VISIBLE_COLS + width) *
				      CONFIG_VIDEO_PIXEL_SIZE;
			}
			break;
		default:
			puts("4bpp bitmap unsupported with current "
			     "video mode\n");
			break;
		}
		break;

	case 8:
		padded_line -= width;
		if (CONFIG_VIDEO_DATA_FORMAT == GDF__8BIT_INDEX) {
			/* Copy colormap */
			for (xcount = 0; xcount < colors; ++xcount) {
				cte = bmp->color_table[xcount];
				video_set_lut (xcount, cte.red, cte.green, cte.blue);
			}
		}
		ycount = height;
		switch (CONFIG_VIDEO_DATA_FORMAT) {
		case GDF__8BIT_INDEX:
			while (ycount--) {
				WATCHDOG_RESET ();
				xcount = width;
				while (xcount--) {
					*fb++ = *bmap++;
				}
				bmap += padded_line;
				fb -= (CONFIG_VIDEO_VISIBLE_COLS + width) * CONFIG_VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF__8BIT_332RGB:
			while (ycount--) {
				WATCHDOG_RESET ();
				xcount = width;
				while (xcount--) {
					cte = bmp->color_table[*bmap++];
					FILL_8BIT_332RGB (cte.red, cte.green, cte.blue);
				}
				bmap += padded_line;
				fb -= (CONFIG_VIDEO_VISIBLE_COLS + width) * CONFIG_VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_15BIT_555RGB:
			while (ycount--) {
#if defined(VIDEO_FB_16BPP_PIXEL_SWAP)
				int xpos = x;
#endif
				WATCHDOG_RESET ();
				xcount = width;
				while (xcount--) {
					cte = bmp->color_table[*bmap++];
#if defined(VIDEO_FB_16BPP_PIXEL_SWAP)
					fill_555rgb_pswap (fb, xpos++, cte.red,
							   cte.green, cte.blue);
					fb += 2;
#else
					FILL_15BIT_555RGB (cte.red, cte.green, cte.blue);
#endif
				}
				bmap += padded_line;
				fb -= (CONFIG_VIDEO_VISIBLE_COLS + width) * CONFIG_VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_16BIT_565RGB:
			while (ycount--) {
				WATCHDOG_RESET ();
				xcount = width;
				while (xcount--) {
					cte = bmp->color_table[*bmap++];
					FILL_16BIT_565RGB (cte.red, cte.green, cte.blue);
				}
				bmap += padded_line;
				fb -= (CONFIG_VIDEO_VISIBLE_COLS + width) * CONFIG_VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_32BIT_X888RGB:
			while (ycount--) {
				WATCHDOG_RESET ();
				xcount = width;
				while (xcount--) {
					cte = bmp->color_table[*bmap++];
					FILL_32BIT_X888RGB (cte.red, cte.green, cte.blue);
				}
				bmap += padded_line;
				fb -= (CONFIG_VIDEO_VISIBLE_COLS + width) * CONFIG_VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_24BIT_888RGB:
			while (ycount--) {
				WATCHDOG_RESET ();
				xcount = width;
				while (xcount--) {
					cte = bmp->color_table[*bmap++];
					FILL_24BIT_888RGB (cte.red, cte.green, cte.blue);
				}
				bmap += padded_line;
				fb -= (CONFIG_VIDEO_VISIBLE_COLS + width) * CONFIG_VIDEO_PIXEL_SIZE;
			}
			break;
		}
		break;
	case 24:
		padded_line -= 3 * width;
		ycount = height;
		switch (CONFIG_VIDEO_DATA_FORMAT) {
		case GDF__8BIT_332RGB:
			while (ycount--) {
				WATCHDOG_RESET ();
				xcount = width;
				while (xcount--) {
					FILL_8BIT_332RGB (bmap[2], bmap[1], bmap[0]);
					bmap += 3;
				}
				bmap += padded_line;
				fb -= (CONFIG_VIDEO_VISIBLE_COLS + width) * CONFIG_VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_15BIT_555RGB:
			while (ycount--) {
#if defined(VIDEO_FB_16BPP_PIXEL_SWAP)
				int xpos = x;
#endif
				WATCHDOG_RESET ();
				xcount = width;
				while (xcount--) {
#if defined(VIDEO_FB_16BPP_PIXEL_SWAP)
					fill_555rgb_pswap (fb, xpos++, bmap[2],
							   bmap[1], bmap[0]);
					fb += 2;
#else
					FILL_15BIT_555RGB (bmap[2], bmap[1], bmap[0]);
#endif
					bmap += 3;
				}
				bmap += padded_line;
				fb -= (CONFIG_VIDEO_VISIBLE_COLS + width) * CONFIG_VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_16BIT_565RGB:
			while (ycount--) {
				WATCHDOG_RESET ();
				xcount = width;
				while (xcount--) {
					FILL_16BIT_565RGB (bmap[2], bmap[1], bmap[0]);
					bmap += 3;
				}
				bmap += padded_line;
				fb -= (CONFIG_VIDEO_VISIBLE_COLS + width) * CONFIG_VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_32BIT_X888RGB:
			while (ycount--) {
				WATCHDOG_RESET ();
				xcount = width;
				while (xcount--) {
					FILL_32BIT_X888RGB (bmap[2], bmap[1], bmap[0]);
					bmap += 3;
				}
				bmap += padded_line;
				fb -= (CONFIG_VIDEO_VISIBLE_COLS + width) * CONFIG_VIDEO_PIXEL_SIZE;
			}
			break;
		case GDF_24BIT_888RGB:
			while (ycount--) {
				WATCHDOG_RESET ();
				xcount = width;
				while (xcount--) {
					FILL_24BIT_888RGB (bmap[2], bmap[1], bmap[0]);
					bmap += 3;
				}
				bmap += padded_line;
				fb -= (CONFIG_VIDEO_VISIBLE_COLS + width) * CONFIG_VIDEO_PIXEL_SIZE;
			}
			break;
		default:
			printf ("Error: 24 bits/pixel bitmap incompatible with current video mode\n");
			break;
		}
		break;
	default:
		printf ("Error: %d bit/pixel bitmaps not supported by U-Boot\n",
			le16_to_cpu (bmp->header.bit_count));
		break;
	}

#ifdef CONFIG_VIDEO_BMP_GZIP
	if (dst) {
		free(dst);
	}
#endif

	return (0);
}
#endif

/*****************************************************************************/

#ifdef CONFIG_VIDEO_LOGO
void logo_plot (void *screen, int width, int x, int y)
{

	int xcount, i;
	int skip   = (width - VIDEO_LOGO_WIDTH) * CONFIG_VIDEO_PIXEL_SIZE;
	int ycount = video_logo_height;
	unsigned char r, g, b, *logo_red, *logo_blue, *logo_green;
	unsigned char *source;
	unsigned char *dest = (unsigned char *)screen +
			      ((y * width * CONFIG_VIDEO_PIXEL_SIZE) +
			       x * CONFIG_VIDEO_PIXEL_SIZE);

#ifdef CONFIG_VIDEO_BMP_LOGO
	source = bmp_logo_bitmap;

	/* Allocate temporary space for computing colormap */
	logo_red = malloc (BMP_LOGO_COLORS);
	logo_green = malloc (BMP_LOGO_COLORS);
	logo_blue = malloc (BMP_LOGO_COLORS);
	/* Compute color map */
	for (i = 0; i < VIDEO_LOGO_COLORS; i++) {
		logo_red[i] = (bmp_logo_palette[i] & 0x0f00) >> 4;
		logo_green[i] = (bmp_logo_palette[i] & 0x00f0);
		logo_blue[i] = (bmp_logo_palette[i] & 0x000f) << 4;
	}
#else
	source = linux_logo;
	logo_red = linux_logo_red;
	logo_green = linux_logo_green;
	logo_blue = linux_logo_blue;
#endif

	if (CONFIG_VIDEO_DATA_FORMAT == GDF__8BIT_INDEX) {
		for (i = 0; i < VIDEO_LOGO_COLORS; i++) {
			video_set_lut (i + VIDEO_LOGO_LUT_OFFSET,
				       logo_red[i], logo_green[i], logo_blue[i]);
		}
	}

	while (ycount--) {
#if defined(VIDEO_FB_16BPP_PIXEL_SWAP)
		int xpos = x;
#endif
		xcount = VIDEO_LOGO_WIDTH;
		while (xcount--) {
			r = logo_red[*source - VIDEO_LOGO_LUT_OFFSET];
			g = logo_green[*source - VIDEO_LOGO_LUT_OFFSET];
			b = logo_blue[*source - VIDEO_LOGO_LUT_OFFSET];

			switch (CONFIG_VIDEO_DATA_FORMAT) {
			case GDF__8BIT_INDEX:
				*dest = *source;
				break;
			case GDF__8BIT_332RGB:
				*dest = ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6);
				break;
			case GDF_15BIT_555RGB:
#if defined(VIDEO_FB_16BPP_PIXEL_SWAP)
				fill_555rgb_pswap (dest, xpos++, r, g, b);
#else
				*(unsigned short *) dest =
					SWAP16 ((unsigned short) (((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3)));
#endif
				break;
			case GDF_16BIT_565RGB:
				*(unsigned short *) dest =
					SWAP16 ((unsigned short) (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)));
				break;
			case GDF_32BIT_X888RGB:
				*(unsigned long *) dest =
					SWAP32 ((unsigned long) ((r << 16) | (g << 8) | b));
				break;
			case GDF_24BIT_888RGB:
#ifdef VIDEO_FB_LITTLE_ENDIAN
				dest[0] = b;
				dest[1] = g;
				dest[2] = r;
#else
				dest[0] = r;
				dest[1] = g;
				dest[2] = b;
#endif
				break;
			}
			source++;
			dest += CONFIG_VIDEO_PIXEL_SIZE;
		}
		dest += skip;
	}
#ifdef CONFIG_VIDEO_BMP_LOGO
	free (logo_red);
	free (logo_green);
	free (logo_blue);
#endif
}

/*****************************************************************************/

static void *video_logo (void)
{
	char info[128];
	extern char version_string;
	int space, len, y_off = 0;

#ifdef CONFIG_SPLASH_SCREEN
	char *s;
	ulong addr;

	if ((s = getenv ("splashimage")) != NULL) {
		int x = 0, y = 0;

		addr = simple_strtoul (s, NULL, 16);
#ifdef CONFIG_SPLASH_SCREEN_ALIGN
		if ((s = getenv ("splashpos")) != NULL) {
			if (s[0] == 'm')
				x = BMP_ALIGN_CENTER;
			else
				x = simple_strtol (s, NULL, 0);

			if ((s = strchr (s + 1, ',')) != NULL) {
				if (s[1] == 'm')
					y = BMP_ALIGN_CENTER;
				else
					y = simple_strtol (s + 1, NULL, 0);
			}
		}
#endif /* CONFIG_SPLASH_SCREEN_ALIGN */

		if (video_display_bitmap (addr, x, y) == 0) {
			video_logo_height = 0;
			return ((void *) (video_fb_address));
		}
	}
#endif /* CONFIG_SPLASH_SCREEN */

	logo_plot (video_fb_address, VIDEO_COLS, 0, 0);

	sprintf (info, " %s", &version_string);

	space = (VIDEO_LINE_LEN / 2 - VIDEO_INFO_X) / VIDEO_FONT_WIDTH;
	len = strlen(info);

	if (len > space) {
		video_drawchars (VIDEO_INFO_X, VIDEO_INFO_Y,
				 (uchar *)info, space);
		video_drawchars (VIDEO_INFO_X + VIDEO_FONT_WIDTH,
				 VIDEO_INFO_Y + VIDEO_FONT_HEIGHT,
				 (uchar *)info + space, len - space);
		y_off = 1;
	} else
		video_drawstring (VIDEO_INFO_X, VIDEO_INFO_Y, (uchar *)info);

#ifdef CONFIG_CONSOLE_EXTRA_INFO
	{
		int i, n = ((video_logo_height - VIDEO_FONT_HEIGHT) / VIDEO_FONT_HEIGHT);

		for (i = 1; i < n; i++) {
			video_get_info_str (i, info);
			if (!*info)
				continue;

			len = strlen(info);
			if (len > space) {
				video_drawchars (VIDEO_INFO_X,
						 VIDEO_INFO_Y +
						 (i + y_off) * VIDEO_FONT_HEIGHT,
						 (uchar *)info, space);
				y_off++;
				video_drawchars (VIDEO_INFO_X + VIDEO_FONT_WIDTH,
						 VIDEO_INFO_Y +
						 (i + y_off) * VIDEO_FONT_HEIGHT,
						 (uchar *)info + space,
						 len - space);
			} else {
				video_drawstring (VIDEO_INFO_X,
						  VIDEO_INFO_Y +
						  (i + y_off) * VIDEO_FONT_HEIGHT,
						  (uchar *)info);
			}
		}
	}
#endif

	return (video_fb_address + video_logo_height * VIDEO_LINE_LEN);
}
#endif


/*****************************************************************************/

int video_resume(void *videobase, int row, int col) {
	unsigned char color8;

	video_fb_address = videobase;
#ifdef CONFIG_VIDEO_HW_CURSOR
	video_init_hw_cursor (VIDEO_FONT_WIDTH, VIDEO_FONT_HEIGHT);
#endif

	/* Init drawing pats */
	switch (CONFIG_VIDEO_DATA_FORMAT) {
	case GDF__8BIT_INDEX:
		video_set_lut (0x01, CONSOLE_FG_COL, CONSOLE_FG_COL, CONSOLE_FG_COL);
		video_set_lut (0x00, CONSOLE_BG_COL, CONSOLE_BG_COL, CONSOLE_BG_COL);
		fgx = 0x01010101;
		bgx = 0x00000000;
		break;
	case GDF__8BIT_332RGB:
		color8 = ((CONSOLE_FG_COL & 0xe0) |
			  ((CONSOLE_FG_COL >> 3) & 0x1c) | CONSOLE_FG_COL >> 6);
		fgx = (color8 << 24) | (color8 << 16) | (color8 << 8) | color8;
		color8 = ((CONSOLE_BG_COL & 0xe0) |
			  ((CONSOLE_BG_COL >> 3) & 0x1c) | CONSOLE_BG_COL >> 6);
		bgx = (color8 << 24) | (color8 << 16) | (color8 << 8) | color8;
		break;
	case GDF_15BIT_555RGB:
		fgx = (((CONSOLE_FG_COL >> 3) << 26) |
		       ((CONSOLE_FG_COL >> 3) << 21) | ((CONSOLE_FG_COL >> 3) << 16) |
		       ((CONSOLE_FG_COL >> 3) << 10) | ((CONSOLE_FG_COL >> 3) << 5) |
		       (CONSOLE_FG_COL >> 3));
		bgx = (((CONSOLE_BG_COL >> 3) << 26) |
		       ((CONSOLE_BG_COL >> 3) << 21) | ((CONSOLE_BG_COL >> 3) << 16) |
		       ((CONSOLE_BG_COL >> 3) << 10) | ((CONSOLE_BG_COL >> 3) << 5) |
		       (CONSOLE_BG_COL >> 3));
		break;
	case GDF_16BIT_565RGB:
		fgx = (((CONSOLE_FG_COL >> 3) << 27) |
		       ((CONSOLE_FG_COL >> 2) << 21) | ((CONSOLE_FG_COL >> 3) << 16) |
		       ((CONSOLE_FG_COL >> 3) << 11) | ((CONSOLE_FG_COL >> 2) << 5) |
		       (CONSOLE_FG_COL >> 3));
		bgx = (((CONSOLE_BG_COL >> 3) << 27) |
		       ((CONSOLE_BG_COL >> 2) << 21) | ((CONSOLE_BG_COL >> 3) << 16) |
		       ((CONSOLE_BG_COL >> 3) << 11) | ((CONSOLE_BG_COL >> 2) << 5) |
		       (CONSOLE_BG_COL >> 3));
		break;
	case GDF_32BIT_X888RGB:
		fgx = (CONSOLE_FG_COL << 16) | (CONSOLE_FG_COL << 8) | CONSOLE_FG_COL;
		bgx = (CONSOLE_BG_COL << 16) | (CONSOLE_BG_COL << 8) | CONSOLE_BG_COL;
		break;
	case GDF_24BIT_888RGB:
		fgx = (CONSOLE_FG_COL << 24) | (CONSOLE_FG_COL << 16) |
			(CONSOLE_FG_COL << 8) | CONSOLE_FG_COL;
		bgx = (CONSOLE_BG_COL << 24) | (CONSOLE_BG_COL << 16) |
			(CONSOLE_BG_COL << 8) | CONSOLE_BG_COL;
		break;
	}
	eorx = fgx ^ bgx;

#ifdef CONFIG_VIDEO_LOGO
	/* Plot the logo and get start point of console */
	PRINTD ("Video: Drawing the logo ...\n");
	video_console_address = video_logo ();
#else
	video_console_address = video_fb_address;
#endif

	/* Initialize the console */
	console_col = col;
	console_row = row;

	return 0;
}

int video_get_col(void) {
    return console_col;
}

int video_get_row(void) {
    return console_row;
}

int video_init (void *videobase)
{
	unsigned char color8;

	video_fb_address = videobase;
#ifdef CONFIG_VIDEO_HW_CURSOR
	video_init_hw_cursor (VIDEO_FONT_WIDTH, VIDEO_FONT_HEIGHT);
#endif

	/* Init drawing pats */
	switch (CONFIG_VIDEO_DATA_FORMAT) {
	case GDF__8BIT_INDEX:
		video_set_lut (0x01, CONSOLE_FG_COL, CONSOLE_FG_COL, CONSOLE_FG_COL);
		video_set_lut (0x00, CONSOLE_BG_COL, CONSOLE_BG_COL, CONSOLE_BG_COL);
		fgx = 0x01010101;
		bgx = 0x00000000;
		break;
	case GDF__8BIT_332RGB:
		color8 = ((CONSOLE_FG_COL & 0xe0) |
			  ((CONSOLE_FG_COL >> 3) & 0x1c) | CONSOLE_FG_COL >> 6);
		fgx = (color8 << 24) | (color8 << 16) | (color8 << 8) | color8;
		color8 = ((CONSOLE_BG_COL & 0xe0) |
			  ((CONSOLE_BG_COL >> 3) & 0x1c) | CONSOLE_BG_COL >> 6);
		bgx = (color8 << 24) | (color8 << 16) | (color8 << 8) | color8;
		break;
	case GDF_15BIT_555RGB:
		fgx = (((CONSOLE_FG_COL >> 3) << 26) |
		       ((CONSOLE_FG_COL >> 3) << 21) | ((CONSOLE_FG_COL >> 3) << 16) |
		       ((CONSOLE_FG_COL >> 3) << 10) | ((CONSOLE_FG_COL >> 3) << 5) |
		       (CONSOLE_FG_COL >> 3));
		bgx = (((CONSOLE_BG_COL >> 3) << 26) |
		       ((CONSOLE_BG_COL >> 3) << 21) | ((CONSOLE_BG_COL >> 3) << 16) |
		       ((CONSOLE_BG_COL >> 3) << 10) | ((CONSOLE_BG_COL >> 3) << 5) |
		       (CONSOLE_BG_COL >> 3));
		break;
	case GDF_16BIT_565RGB:
		fgx = (((CONSOLE_FG_COL >> 3) << 27) |
		       ((CONSOLE_FG_COL >> 2) << 21) | ((CONSOLE_FG_COL >> 3) << 16) |
		       ((CONSOLE_FG_COL >> 3) << 11) | ((CONSOLE_FG_COL >> 2) << 5) |
		       (CONSOLE_FG_COL >> 3));
		bgx = (((CONSOLE_BG_COL >> 3) << 27) |
		       ((CONSOLE_BG_COL >> 2) << 21) | ((CONSOLE_BG_COL >> 3) << 16) |
		       ((CONSOLE_BG_COL >> 3) << 11) | ((CONSOLE_BG_COL >> 2) << 5) |
		       (CONSOLE_BG_COL >> 3));
		break;
	case GDF_32BIT_X888RGB:
		fgx = (CONSOLE_FG_COL << 16) | (CONSOLE_FG_COL << 8) | CONSOLE_FG_COL;
		bgx = (CONSOLE_BG_COL << 16) | (CONSOLE_BG_COL << 8) | CONSOLE_BG_COL;
		break;
	case GDF_24BIT_888RGB:
		fgx = (CONSOLE_FG_COL << 24) | (CONSOLE_FG_COL << 16) |
			(CONSOLE_FG_COL << 8) | CONSOLE_FG_COL;
		bgx = (CONSOLE_BG_COL << 24) | (CONSOLE_BG_COL << 16) |
			(CONSOLE_BG_COL << 8) | CONSOLE_BG_COL;
		break;
	}
	eorx = fgx ^ bgx;

#ifdef CONFIG_VIDEO_LOGO
	/* Plot the logo and get start point of console */
	PRINTD ("Video: Drawing the logo ...\n");
	video_console_address = video_logo ();
#else
	video_console_address = video_fb_address;
#endif

	/* Initialize the console */
	console_col = 0;
	console_row = 0;

	memsetl(CONSOLE_ROW_FIRST, VIDEO_COLS * VIDEO_ROWS,
		CONSOLE_BG_COL);

	return 0;
}
