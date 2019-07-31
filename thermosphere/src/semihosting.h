#pragma once

// Adapted from Arm TF

/*
 * Copyright (c) 2013-2014, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdio.h> /* For ssize_t */


#define SEMIHOSTING_SYS_OPEN            0x01
#define SEMIHOSTING_SYS_CLOSE           0x02
#define SEMIHOSTING_SYS_WRITE0          0x04
#define SEMIHOSTING_SYS_WRITEC          0x03
#define SEMIHOSTING_SYS_WRITE           0x05
#define SEMIHOSTING_SYS_READ            0x06
#define SEMIHOSTING_SYS_READC           0x07
#define SEMIHOSTING_SYS_SEEK            0x0A
#define SEMIHOSTING_SYS_FLEN            0x0C
#define SEMIHOSTING_SYS_REMOVE          0x0E
#define SEMIHOSTING_SYS_SYSTEM          0x12
#define SEMIHOSTING_SYS_ERRNO           0x13

#define FOPEN_MODE_R            0x0
#define FOPEN_MODE_RB           0x1
#define FOPEN_MODE_RPLUS        0x2
#define FOPEN_MODE_RPLUSB       0x3
#define FOPEN_MODE_W            0x4
#define FOPEN_MODE_WB           0x5
#define FOPEN_MODE_WPLUS        0x6
#define FOPEN_MODE_WPLUSB       0x7
#define FOPEN_MODE_A            0x8
#define FOPEN_MODE_AB           0x9
#define FOPEN_MODE_APLUS        0xa
#define FOPEN_MODE_APLUSB       0xb

static inline long semihosting_connection_supported(void)
{
#ifdef PLATFORM_QEMU
    return 1;
#else
    return 0;
#endif
}

long semihosting_file_open(const char *file_name, size_t mode);
long semihosting_file_seek(long file_handle, ssize_t offset);
long semihosting_file_read(long file_handle, size_t *length, uintptr_t buffer);
long semihosting_file_write(long file_handle,
                size_t *length,
                const uintptr_t buffer);
long semihosting_file_close(long file_handle);
long semihosting_file_length(long file_handle);
long semihosting_system(char *command_line);
long semihosting_get_flen(const char *file_name);
long semihosting_download_file(const char *file_name,
                   size_t buf_size,
                   uintptr_t buf);
void semihosting_write_char(char character);
void semihosting_write_string(const char *string);
char semihosting_read_char(void);
