#ifndef FUSEE_EXCEPTION_HANDLERS_H
#define FUSEE_EXCEPTION_HANDLERS_H

#include <stdint.h>
#include <stddef.h>

/* Copies up to len bytes, stops and returns the read length on data fault. */
size_t safecpy(void *dst, const void *src, size_t len);

void setup_exception_handlers(void);

#endif
