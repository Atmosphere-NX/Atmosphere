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
	char buf[512];
	va_start(list, fmt);
	vsnprintf(buf, sizeof(buf), fmt, list);
	video_puts(buf);
	va_end(list);
}
