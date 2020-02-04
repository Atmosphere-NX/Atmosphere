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

/* Adapted from Luma3DS with permission. */

/* File : barebones/ee_printf.c
	This file contains an implementation of ee_printf that only requires a method to output a char to a UART without pulling in library code.
This code is based on a file that contains the following:
 Copyright (C) 2002 Michael Ringgaard. All rights reserved.
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
 3. Neither the name of the project nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 SUCH DAMAGE.
*/

//TuxSH's changes: add support for 64-bit numbers, remove floating-point code

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>

#define ZEROPAD   (1<<0) //Pad with zero
#define SIGN      (1<<1) //Unsigned/signed long
#define PLUS      (1<<2) //Show plus
#define SPACE     (1<<3) //Spacer
#define LEFT      (1<<4) //Left justified
#define HEX_PREP  (1<<5) //0x
#define UPPERCASE (1<<6) //'ABCDEF'

#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')

static int skipAtoi(const char **s)
{
    int i = 0;

    while(IS_DIGIT(**s)) i = i * 10 + *((*s)++) - '0';

    return i;
}

static char *processNumber(char *str, long long num, bool isHex, int size, int precision, unsigned int type)
{
    char sign = 0;

    if(type & SIGN)
    {
        if(num < 0)
        {
            sign = '-';
            num = -num;
            size--;
        }
        else if(type & PLUS)
        {
            sign = '+';
            size--;
        }
        else if(type & SPACE)
        {
            sign = ' ';
            size--;
        }
    }

    static const char *lowerDigits = "0123456789abcdef",
                      *upperDigits = "0123456789ABCDEF";

    int i = 0;
    char tmp[20];
    const char *dig = (type & UPPERCASE) ? upperDigits : lowerDigits;

    if(num == 0)
    {
        if(precision != 0) tmp[i++] = '0';
        type &= ~HEX_PREP;
    }
    else
    {
        while(num != 0)
        {
            unsigned long long int base = isHex ? 16ULL : 10ULL;
            tmp[i++] = dig[(unsigned long long int)num % base];
            num = (long long)((unsigned long long int)num / base);
        }
    }

    if(type & LEFT || precision != -1) type &= ~ZEROPAD;
    if(type & HEX_PREP && isHex) size -= 2;
    if(i > precision) precision = i;
    size -= precision;
    if(!(type & (ZEROPAD | LEFT))) while(size-- > 0) *str++ = ' ';
    if(sign) *str++ = sign;

    if(type & HEX_PREP && isHex)
    {
        *str++ = '0';
        *str++ = 'x';
    }

    if(type & ZEROPAD) while(size-- > 0) *str++ = '0';
    while(i < precision--) *str++ = '0';
    while(i-- > 0) *str++ = tmp[i];
    while(size-- > 0) *str++ = ' ';

    return str;
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
    char *str;

    for(str = buf; *fmt; fmt++)
    {
        if(*fmt != '%')
        {
            *str++ = *fmt;
            continue;
        }

        //Process flags
        unsigned int flags = 0; //Flags to number()
        bool loop = true;

        while(loop)
        {
            switch(*++fmt)
            {
                case '-': flags |= LEFT; break;
                case '+': flags |= PLUS; break;
                case ' ': flags |= SPACE; break;
                case '#': flags |= HEX_PREP; break;
                case '0': flags |= ZEROPAD; break;
                default: loop = false; break;
            }
        }

        //Get field width
        int fieldWidth = -1; //Width of output field
        if(IS_DIGIT(*fmt)) fieldWidth = skipAtoi(&fmt);
        else if(*fmt == '*')
        {
            fmt++;

            fieldWidth = va_arg(args, int);

            if(fieldWidth < 0)
            {
                fieldWidth = -fieldWidth;
                flags |= LEFT;
            }
        }

        //Get the precision
        int precision = -1; //Min. # of digits for integers; max number of chars for from string
        if(*fmt == '.')
        {
            fmt++;

            if(IS_DIGIT(*fmt)) precision = skipAtoi(&fmt);
            else if(*fmt == '*')
            {
                fmt++;
                precision = va_arg(args, int);
            }

            if(precision < 0) precision = 0;
        }

        //Get the conversion qualifier
        unsigned int integerType = 0;
        if(*fmt == 'l')
        {
            if(*++fmt == 'l')
            {
                fmt++;
                integerType = 1;
            }
            else
            {
                integerType = sizeof(unsigned long) == 8 ? 1 : 0;
            }
        }
        else if(*fmt == 'h')
        {
            if(*++fmt == 'h')
            {
                fmt++;
                integerType = 3;
            }
            else integerType = 2;
        }

        bool isHex;

        switch(*fmt)
        {
            case 'c':
                if(!(flags & LEFT)) while(--fieldWidth > 0) *str++ = ' ';
                *str++ = (unsigned char)va_arg(args, int);
                while(--fieldWidth > 0) *str++ = ' ';
                continue;

            case 's':
            {
                char *s = va_arg(args, char *);
                if(!s) s = "<NULL>";
                unsigned int len = (precision != -1) ? strnlen(s, precision) : strlen(s);
                if(!(flags & LEFT)) while((int)len < fieldWidth--) *str++ = ' ';
                for(unsigned int i = 0; i < len; i++) *str++ = *s++;
                while((int)len < fieldWidth--) *str++ = ' ';
                continue;
            }

            case 'p':
                if(fieldWidth == -1)
                {
                    fieldWidth = 8;
                    flags |= ZEROPAD;
                }
                str = processNumber(str, va_arg(args, unsigned int), true, fieldWidth, precision, flags);
                continue;

            //Integer number formats - set up the flags and "break"
            case 'X':
                flags |= UPPERCASE;
                //Falls through
            case 'x':
                isHex = true;
                break;

            case 'd':
            case 'i':
                flags |= SIGN;
                //Falls through
            case 'u':
                isHex = false;
                break;

            default:
                if(*fmt != '%') *str++ = '%';
                if(*fmt) *str++ = *fmt;
                else fmt--;
                continue;
        }

        long long num;

        if(flags & SIGN)
        {
            if(integerType == 1) num = va_arg(args, signed long long int);
            else num = va_arg(args, signed int);

            if(integerType == 2) num = (signed short)num;
            else if(integerType == 3) num = (signed char)num;
        }
        else
        {
            if(integerType == 1) num = va_arg(args, unsigned long long int);
            else num = va_arg(args, unsigned int);

            if(integerType == 2) num = (unsigned short)num;
            else if(integerType == 3) num = (unsigned char)num;
        }

        str = processNumber(str, num, isHex, fieldWidth, precision, flags);
    }

    *str = 0;
    return str - buf;
}

int sprintf(char *buf, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int res = vsprintf(buf, fmt, args);
    va_end(args);
    return res;
}


#ifdef __cplusplus
}
#endif