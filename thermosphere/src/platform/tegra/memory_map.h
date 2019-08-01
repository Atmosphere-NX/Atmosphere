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

#include "../../types.h"

static inline u64 transformKernelAddress(u64 pa)
{
    switch (pa) {
        // GICv2 CPU -> vCPU interface
        case 0x50042000:
            return 0x50046000;
        default:
            return pa;
    }
}

uintptr_t configureMemoryMap(u32 *addrSpaceSize);
