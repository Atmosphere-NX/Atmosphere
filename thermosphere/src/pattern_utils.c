/*
 * Copyright (c) 2019 Atmosphère-NX
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

#include <string.h>
#include "pattern_utils.h"


//Boyer-Moore Horspool algorithm, adapted from http://www-igm.univ-mlv.fr/~lecroq/string/node18.html#SECTION00180
//u32 size to limit stack usage
u8 *memsearch(u8 *startPos, const void *pattern, u32 size, u32 patternSize)
{
    const u8 *patternc = (const u8 *)pattern;
    u32 table[256];

    //Preprocessing
    for(u32 i = 0; i < 256; i++)
        table[i] = patternSize;
    for(u32 i = 0; i < patternSize - 1; i++)
        table[patternc[i]] = patternSize - i - 1;

    //Searching
    u32 j = 0;
    while(j <= size - patternSize)
    {
        u8 c = startPos[j + patternSize - 1];
        if(patternc[patternSize - 1] == c && memcmp(pattern, startPos + j, patternSize - 1) == 0)
            return startPos + j;
        j += table[c];
    }

    return NULL;
}

// Copied from newlib, without the reent stuff + some other stuff

/*
 * Copyright (c) 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

unsigned int xstrtoui(const char *nptr, char **endptr, int base, bool allowPrefix, bool *ok)
{
    register const unsigned char *s = (const unsigned char *)nptr;
    register unsigned int acc;
    register int c;
    register unsigned int cutoff;
    register int neg = 0, any, cutlim;

    if(ok != NULL)
        *ok = true;
    /*
     * See strtol for comments as to the logic used.
     */
    do {
        c = *s++;
    } while ((c >= 9 && c <= 13) || c == ' ');
    if (c == '-') {
        if(!allowPrefix) {
            if(ok != NULL)
                *ok = false;
            return 0;
        }
        neg = 1;
        c = *s++;
    } else if (c == '+'){
        if(!allowPrefix) {
            if(ok != NULL)
                *ok = false;
            return 0;
        }

        c = *s++;
    }
    if ((base == 0 || base == 16) &&
        c == '0' && (*s == 'x' || *s == 'X')) {

        if(!allowPrefix) {
            if(ok != NULL)
                *ok = false;
            return 0;
        }
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0) {
        if(!allowPrefix) {
            if(ok != NULL)
                *ok = false;
            return 0;
        }

        base = c == '0' ? 8 : 10;
    }
    cutoff = (unsigned int)(-1) / (unsigned int)base;
    cutlim = (unsigned int)(-1) % (unsigned int)base;
    for (acc = 0, any = 0;; c = *s++) {
        if (c >= '0' && c <= '9')
            c -= '0';
        else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
            c -= c >= 'A' && c <= 'Z' ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
               if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
            any = -1;
        else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }
    if (any < 0) {
        acc = (unsigned int)-1;
        if(ok != NULL)
            *ok = false;
//        rptr->_errno = ERANGE;
    } else if (neg)
        acc = -acc;
    if (endptr != 0)
        *endptr = (char *) (any ? (char *)s - 1 : nptr);
    return (acc);
}

// Copied from newlib, without the reent stuff + some other stuff
unsigned long int xstrtoul(const char *nptr, char **endptr, int base, bool allowPrefix, bool *ok)
{
    register const unsigned char *s = (const unsigned char *)nptr;
    register unsigned long acc;
    register int c;
    register unsigned long cutoff;
    register int neg = 0, any, cutlim;

    if(ok != NULL)
        *ok = true;
    /*
     * See strtol for comments as to the logic used.
     */
    do {
        c = *s++;
    } while ((c >= 9 && c <= 13) || c == ' ');
    if (c == '-') {
        if(!allowPrefix) {
            if(ok != NULL)
                *ok = false;
            return 0;
        }
        neg = 1;
        c = *s++;
    } else if (c == '+'){
        if(!allowPrefix) {
            if(ok != NULL)
                *ok = false;
            return 0;
        }

        c = *s++;
    }
    if ((base == 0 || base == 16) &&
        c == '0' && (*s == 'x' || *s == 'X')) {

        if(!allowPrefix) {
            if(ok != NULL)
                *ok = false;
            return 0;
        }
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0) {
        if(!allowPrefix) {
            if(ok != NULL)
                *ok = false;
            return 0;
        }

        base = c == '0' ? 8 : 10;
    }
    cutoff = (unsigned long)(-1) / (unsigned long)base;
    cutlim = (unsigned long)(-1) % (unsigned long)base;
    for (acc = 0, any = 0;; c = *s++) {
        if (c >= '0' && c <= '9')
            c -= '0';
        else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
            c -= c >= 'A' && c <= 'Z' ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
               if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
            any = -1;
        else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }
    if (any < 0) {
        acc = (unsigned long)-1;
        if(ok != NULL)
            *ok = false;
//        rptr->_errno = ERANGE;
    } else if (neg)
        acc = -acc;
    if (endptr != 0)
        *endptr = (char *) (any ? (char *)s - 1 : nptr);
    return (acc);
}

// Copied from newlib, without the reent stuff + some other stuff
unsigned long long int xstrtoull(const char *nptr, char **endptr, int base, bool allowPrefix, bool *ok)
{
    register const unsigned char *s = (const unsigned char *)nptr;
    register unsigned long long acc;
    register int c;
    register unsigned long long cutoff;
    register int neg = 0, any, cutlim;

    if(ok != NULL)
        *ok = true;
    /*
     * See strtol for comments as to the logic used.
     */
    do {
        c = *s++;
    } while ((c >= 9 && c <= 13) || c == ' ');
    if (c == '-') {
        if(!allowPrefix) {
            if(ok != NULL)
                *ok = false;
            return 0;
        }
        neg = 1;
        c = *s++;
    } else if (c == '+'){
        if(!allowPrefix) {
            if(ok != NULL)
                *ok = false;
            return 0;
        }

        c = *s++;
    }
    if ((base == 0 || base == 16) &&
        c == '0' && (*s == 'x' || *s == 'X')) {

        if(!allowPrefix) {
            if(ok != NULL)
                *ok = false;
            return 0;
        }
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0) {
        if(!allowPrefix) {
            if(ok != NULL)
                *ok = false;
            return 0;
        }

        base = c == '0' ? 8 : 10;
    }
    cutoff = (unsigned long long)(-1ull) / (unsigned long long)base;
    cutlim = (unsigned long long)(-1ull) % (unsigned long long)base;
    for (acc = 0, any = 0;; c = *s++) {
        if (c >= '0' && c <= '9')
            c -= '0';
        else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
            c -= c >= 'A' && c <= 'Z' ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
               if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
            any = -1;
        else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }
    if (any < 0) {
        acc = (unsigned long long)-1ull;
        if(ok != NULL)
            *ok = false;
//        rptr->_errno = ERANGE;
    } else if (neg)
        acc = -acc;
    if (endptr != 0)
        *endptr = (char *) (any ? (char *)s - 1 : nptr);
    return (acc);
}
