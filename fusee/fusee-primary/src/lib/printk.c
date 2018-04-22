/**
 * Kernel print functions.
 */

#include "printk.h"

#include "vsprintf.h"
#include "../display/video_fb.h"

/**
 * Temporary stand-in main printk.
 *
 * TODO: This should print via UART, console framebuffer, and to a ring for
 * consumption by Horizon
 */
void printk(char *fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	vprintk(fmt, list);
	va_end(list);
}


void vprintk(char *fmt, va_list args)
{
	char buf[512];
	vsnprintf(buf, sizeof(buf), fmt, args);
	video_puts(buf);
}
