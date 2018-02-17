#ifndef EXOSPHERE_UTILS_H
#define EXOSPHERE_UTILS_H

void panic(void);

unsigned int read32le(const unsigned char *dword, unsigned int offset);
unsigned int read32be(const unsigned char *dword, unsigned int offset);

#endif