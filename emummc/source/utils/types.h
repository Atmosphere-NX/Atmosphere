/*
* Copyright (c) 2018 naehrwert
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

#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdint.h>

#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define OFFSET_OF(t, m) ((u32)&((t *)NULL)->m)
#define CONTAINER_OF(mp, t, mn) ((t *)((u32)mp - OFFSET_OF(t, mn)))

/// Creates a bitmask from a bit number.
#ifndef BIT
#define BIT(n) (1U<<(n))
#endif

typedef signed char s8;
typedef short s16;
typedef short SHORT;
typedef int s32;
typedef int INT;
typedef long LONG;
typedef long long int s64;
typedef unsigned char u8;
typedef unsigned char BYTE;
typedef unsigned short u16;
typedef unsigned short WORD;
typedef unsigned short WCHAR;
typedef unsigned int u32;
typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef unsigned long long QWORD;
typedef unsigned long long int u64;
typedef volatile unsigned char vu8;
typedef volatile unsigned short vu16;
typedef volatile unsigned int vu32;

typedef u32 Handle; ///< Kernel object handle.
typedef u32 Result; ///< Function error code result type.

#ifndef __cplusplus
typedef int bool;
#define true  1
#define false 0
#endif /* __cplusplus */

#define BOOT_CFG_AUTOBOOT_EN (1 << 0)
#define BOOT_CFG_FROM_LAUNCH (1 << 1)
#define BOOT_CFG_SEPT_RUN    (1 << 7)

#define EXTRA_CFG_KEYS    (1 << 0)
#define EXTRA_CFG_PAYLOAD (1 << 1)
#define EXTRA_CFG_MODULE  (1 << 2)

typedef struct __attribute__((__packed__)) _boot_cfg_t
{
	u8  boot_cfg;
	u8  autoboot;
	u8  autoboot_list;
	u8  extra_cfg;
	u8  rsvd[128];
} boot_cfg_t;

typedef struct __attribute__((__packed__)) _ipl_ver_meta_t
{
	u32 magic;
	u32 version;
	u16 rsvd0;
	u16 rsvd1;
} ipl_ver_meta_t;

typedef struct __attribute__((__packed__)) _reloc_meta_t
{
	u32 start;
	u32 stack;
	u32 end;
	u32 ep;
} reloc_meta_t;

#endif
