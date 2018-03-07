#ifndef EXOSPHERE_DBG_FMT_H
#define EXOSPHERE_DBG_FMT_H

#include <stdarg.h>

int visprintf(char *buf, const char *fmt, va_list args);
int isprintf(char *buf, const char *fmt, ...);

#endif
