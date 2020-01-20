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

#pragma once

#include "utils.h"

size_t guestReadWriteMemory(uintptr_t addr, size_t size, void *readBuf, const void *writeBuf);

static inline size_t guestReadMemory(uintptr_t addr, size_t size, void *buf)
{
    return guestReadWriteMemory(addr, size, buf, NULL);
}

static inline size_t guestWriteMemory(uintptr_t addr, size_t size, void *buf)
{
    return guestReadWriteMemory(addr, size, NULL, buf);
}
