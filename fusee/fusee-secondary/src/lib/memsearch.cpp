#include "memsearch.h"
#include <string.h>

bool matchPattern(
    const uint64_t startLocation,
    const unsigned char* pattern,
    const uint32_t patternSize,
    const uint64_t mask)
{
    for (uint32_t i = 0; i < patternSize; i++)
    {
        /* if the bit in our mask is 0 this is a wildcard */
        if ((mask & 1 << i) == 0) continue;

        /* if the byte does not match our pattern we can skip here */
        if (*((unsigned char*)startLocation + i) != pattern[i]) return false;
    }

    return true;
}

uint64_t findPattern(
    const uint64_t startLocation,
    const uint64_t searchAreaSize,
    const unsigned char* pattern,
    const uint32_t patternSize,
    const uint64_t mask,
    unsigned int skipCount,
    const int alignment)
{
    for (uint64_t currentLocation = startLocation; currentLocation + patternSize <= startLocation + searchAreaSize; currentLocation += alignment)
    {
        /*
        If we have aligning code we can skip 2, 4 or 8 bytes each step
        this will speed up this process significantly!
        This has to be keept in mind for pattern generation!
        Patterns can then only be starting at those alignments.
        */

        if (matchPattern(currentLocation, pattern, patternSize, mask))
        {
            /* we found a matching location */
            /* if we have to skip, do so */
            if (skipCount)
            {
                skipCount--;
                continue;
            }

            /* we are done, target found, no more skipping */
            return currentLocation - startLocation;
        }
    }

    return 0;
}

uint64_t makeBinaryMask(const unsigned char* mask, const uint32_t maskSize, const unsigned char* wildcardChar)
{
    uint64_t binaryMask = 0;

    for (uint32_t i = 0; i < maskSize; i++)
    {
        if (mask[i] == *wildcardChar)
        {
            binaryMask |= 1 << i;
        }
    }

    return binaryMask;
}

/*
Searches for the proveded pattern using a char mask and a wildcard character.
It skips a set amount of false positives defined in skipCount.
You can define the byte alignment in memory if your pattern requires it, but you should try to keep the alignment as high as possible (4+).

Returns the offset from startLocation at wich the pattern was found or 0 if not found.
*/
uint64_t memSearchASLR(
    const uint64_t startLocation,
    const uint64_t searchAreaSize,
    const unsigned char* pattern,
    const uint32_t patternSize,
    const unsigned char* mask,
    const uint32_t maskSize,
    const unsigned char* wildcardChar,
    const unsigned int skipCount,
    const unsigned int alignment)
{
    if (searchAreaSize < patternSize) return 0;
    if (patternSize != maskSize) return 0;

    /* check if the pattern fits the binary mask, if this is a problem we can upgrade to unsigned __int128 */
    if (patternSize > sizeof(uint64_t) * 8) return 0;

    const uint64_t binaryMask = makeBinaryMask(mask, maskSize, wildcardChar);

    return findPattern(startLocation, searchAreaSize, pattern, patternSize, binaryMask, skipCount, alignment);
}

/*
Searches for the proveded pattern using a binary mask, use 0 as the wildcard.
It skips a set amount of false positives defined in skipCount.
You can define the byte alignment in memory if your pattern requires it, but you should try to keep the alignment as high as possible (4+).

Returns the offset from startLocation at wich the pattern was found or 0 if not found.
*/
uint64_t memSearchASLR(
    const uint64_t startLocation,
    const uint64_t searchAreaSize,
    const unsigned char* pattern,
    const uint32_t patternSize,
    const uint64_t mask,
    const unsigned int skipCount,
    const unsigned int alignment)
{
    if (searchAreaSize < patternSize) return 0;

    /* check if the pattern fits the binary mask, if this is a problem we can upgrade to unsigned __int128 */
    if (patternSize > sizeof(uint64_t) * 8) return 0;

    return findPattern(startLocation, searchAreaSize, pattern, patternSize, mask, skipCount, alignment);
}

/*
Searches for the proveded pattern and skips a set amount of false positives defined in skipCount.

Returns the offset from startLocation at wich the pattern was found or 0 if not found.
*/
uint64_t memSearch(
    const uint64_t startLocation,
    const uint64_t searchAreaSize,
    const unsigned char* pattern,
    const uint32_t patternSize,
    unsigned int skipCount)
{
    if (patternSize == 0) return 0;
    if (searchAreaSize < patternSize) return 0;

    uint64_t location = 0;
    do
    {
        if (skipCount) skipCount--;
        location = (uint64_t)memmem((const void*)startLocation, searchAreaSize, (const void*)pattern, patternSize);
    } while (skipCount > 0);

    if (location == 0) return 0;

    return location - startLocation;
}
