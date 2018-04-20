/*
 *  Copyright (C) 2011 Andrei Warkentin <andrey.warkentin@gmail.com>
 *
 * This program is free software ; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdarg.h>
#include <stdlib.h>

#ifndef VSPRINTF_H
#define VSPRINTF_H

struct va_format {
        const char *fmt;
        va_list *va;
};

unsigned long long simple_strtoull(const char *cp, char **endp, unsigned int base);

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int sscanf(const char *buf, const char *fmt, ...);

#endif /* VSPRINTF_H */
