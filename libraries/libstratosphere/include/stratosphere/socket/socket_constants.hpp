/*
 * Copyright (c) Atmosphère-NX
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
#include <vapours.hpp>

namespace ams::socket {

    constexpr inline s32 InvalidSocket = -1;
    constexpr inline s32 SocketError   = -1;

    constexpr inline auto DefaultTcpAutoBufferSizeMax      = 192_KB;
    constexpr inline auto MinTransferMemorySize            = (2 * DefaultTcpAutoBufferSizeMax + 128_KB);
    constexpr inline auto MinSocketAllocatorSize           = 128_KB;
    constexpr inline auto MinSocketMemoryPoolSize          = MinSocketAllocatorSize + MinTransferMemorySize;
    constexpr inline auto MinMemHeapAllocatorSize          = 16_KB;
    constexpr inline auto MinimumSharedMbufPoolReservation = 4_KB;

    constexpr inline size_t MemoryPoolAlignment            = 4_KB;

    constexpr inline auto ConcurrencyLimitMax = 14;

    /* TODO: Does this need to be 1 for sockets to work on lower firmware versions? */
    /* Is this value actually used/checked by bsdsockets sysmodule? */
    constexpr inline auto LibraryVersion = 7;

}
