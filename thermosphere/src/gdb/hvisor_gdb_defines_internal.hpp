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

#include "hvisor_gdb_context.hpp"

// 512+24 is the ideal size as IDA will try to read exactly 0x100 bytes at a time.
// IDA seems to want additional bytes as well.
// 1024 is fine enough to put all regs in the 'T' stop reply packets
// Add 4 to this for the actual allocated size, for $#<checksum>, see below.
#define GDB_BUF_LEN                                 0x800
#define GDB_WORK_BUF_LEN                            0x1000

#define GDB_HANDLER(name)                           Handle##name
#define GDB_QUERY_HANDLER(name)                     GDB_HANDLER(Query##name)
#define GDB_VERBOSE_HANDLER(name)                   GDB_HANDLER(Verbose##name)
#define GDB_REMOTE_COMMAND_HANDLER(name)            GDB_HANDLER(RemoteCommand##name)
#define GDB_XFER_HANDLER(name)                      GDB_HANDLER(Xfer##name)

#define GDB_DEFINE_HANDLER(name)                    int Context::GDB_HANDLER(name)()
#define GDB_DEFINE_QUERY_HANDLER(name)              GDB_DEFINE_HANDLER(Query##name)
#define GDB_DEFINE_VERBOSE_HANDLER(name)            GDB_DEFINE_HANDLER(Verbose##name)
#define GDB_DEFINE_REMOTE_COMMAND_HANDLER(name)     GDB_DEFINE_HANDLER(RemoteCommand##name)
#define GDB_DECLARE_XFER_HANDLER(name)              GDB_DEFINE_HANDLER(Xfer##name)

#define GDB_TEST_NO_CMD_DATA()                      do { if (!m_commandData.empty()) return ReplyErrno(EILSEQ); } while (false)
