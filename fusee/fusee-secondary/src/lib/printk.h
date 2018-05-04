#ifndef __PRINTK_H__
#define __PRINTK_H__

#include <stdarg.h>

void printk(char *fmt, ...);
void vprintk(char *fmt, va_list args);

#endif
