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
#include <stdbool.h>

#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define OFFSET_OF(t, m) ((u32)&((t *)NULL)->m)
#define CONTAINER_OF(mp, t, mn) ((t *)((u32)mp - OFFSET_OF(t, mn)))

/// Creates a bitmask from a bit number.
#ifndef BIT
#define BIT(n) (1U<<(n))
#endif

typedef int8_t s8;
typedef int16_t s16;
typedef int16_t SHORT;
typedef int32_t s32;
typedef int32_t INT;
typedef int64_t LONG;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint8_t BYTE;
typedef uint16_t u16;
typedef uint16_t WORD;
typedef uint16_t WCHAR;
typedef uint32_t u32;
typedef uint32_t UINT;
typedef uint32_t DWORD;
typedef uint64_t QWORD;
typedef uint64_t u64;
typedef volatile uint8_t vu8;
typedef volatile uint16_t vu16;
typedef volatile uint32_t vu32;

typedef u32 Handle; ///< Kernel object handle.
typedef u32 Result; ///< Function error code result type.

#define INVALID_HANDLE ((Handle) 0)
#define CUR_PROCESS_HANDLE ((Handle) 0xFFFF8001)

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
