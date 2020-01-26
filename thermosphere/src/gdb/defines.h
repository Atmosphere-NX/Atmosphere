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

// Some code from:
/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "../utils.h"

// 512+24 is the ideal size as IDA will try to read exactly 0x100 bytes at a time.
// IDA seems to want additional bytes as well.
// 1024 is fine enough to put all regs in the 'T' stop reply packets
// Add 4 to this for the actual allocated size, for $#<checksum>, see below.
#define GDB_BUF_LEN 0x800
#define GDB_WORK_BUF_LEN 0x2000

#define GDB_HANDLER(name)           GDB_Handle##name
#define GDB_QUERY_HANDLER(name)     GDB_HANDLER(Query##name)
#define GDB_VERBOSE_HANDLER(name)   GDB_HANDLER(Verbose##name)

#define GDB_DECLARE_HANDLER(name)           int GDB_HANDLER(name)(GDBContext *ctx)
#define GDB_DECLARE_QUERY_HANDLER(name)     GDB_DECLARE_HANDLER(Query##name)
#define GDB_DECLARE_VERBOSE_HANDLER(name)   GDB_DECLARE_HANDLER(Verbose##name)
