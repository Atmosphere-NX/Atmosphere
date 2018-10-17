#ifndef FUSEE_MEMSEARCH_H
#define FUSEE_MEMSEARCH_H

#include <stdint.h>

uint64_t memSearchASLR(
    const uint64_t startLocation,
    const uint64_t searchAreaSize,
    const unsigned char* pattern,
    const uint32_t patternSize,
    const unsigned char* mask,
    const uint32_t maskSize,
    const unsigned char* wildcardChar = (const unsigned char*)'?',
    const unsigned int skipCount = 0,
    const unsigned int alignment = 4);

uint64_t memSearchASLR(
    const uint64_t startLocation,
    const uint64_t searchAreaSize,
    const unsigned char* pattern,
    const uint32_t patternSize,
    const uint64_t mask,
    const unsigned int skipCount = 0,
    const unsigned int alignment = 4);

uint64_t memSearch(
    const uint64_t startLocation,
    const uint64_t searchAreaSize,
    const unsigned char* pattern,
    const uint32_t patternSize,
    unsigned int skipCount = 0);

#endif
