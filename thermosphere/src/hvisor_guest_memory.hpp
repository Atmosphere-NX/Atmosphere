/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include "defines.hpp"

namespace ams::hvisor {

    size_t GuestReadWriteMemory(uintptr_t addr, size_t size, void *readBuf, const void *writeBuf);

    inline size_t GuestReadMemory(uintptr_t addr, size_t size, void *buf)
    {
        return GuestReadWriteMemory(addr, size, buf, NULL);
    }

    inline size_t GuestWriteMemory(uintptr_t addr, size_t size, const void *buf)
    {
        return GuestReadWriteMemory(addr, size, NULL, buf);
    }

}
