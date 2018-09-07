/*
 * Copyright (c) 2018 Atmosphère-NX
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

#include "printk.h"

#include "vsprintf.h"
#include "../display/video_fb.h"

/**
 * Temporary stand-in main printk.
 *
 * TODO: This should print via UART, console framebuffer, and to a ring for
 * consumption by Horizon
 */
void printk(const char *fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	vprintk(fmt, list);
	va_end(list);
}


void vprintk(const char *fmt, va_list args)
{
	char buf[512];
	vsnprintf(buf, sizeof(buf), fmt, args);
	video_puts(buf);
}
