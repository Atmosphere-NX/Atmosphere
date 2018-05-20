#ifndef __PRINTK_H__
#define __PRINTK_H__

#include <stdarg.h>

void printk(const char *fmt, ...);
void vprintk(const char *fmt, va_list args);

#endif
