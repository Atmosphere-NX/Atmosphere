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
#include <stratosphere.hpp>
#include "htclow_driver_memory_management.hpp"

namespace ams::htclow::driver {

    namespace {

        constexpr inline size_t RequiredAlignment = std::max(os::ThreadStackAlignment, os::MemoryPageSize);

        using SocketConfigType = socket::SystemConfigDefault;

        /* TODO: If we ever use resolvers, increase this. */
        constexpr inline size_t SocketAllocatorSize  = 4_KB;
        constexpr inline size_t SocketMemoryPoolSize = util::AlignUp(SocketConfigType::PerTcpSocketWorstCaseMemoryPoolSize + SocketConfigType::PerUdpSocketWorstCaseMemoryPoolSize, os::MemoryPageSize);

        constexpr inline size_t SocketRequiredSize   = util::AlignUp(SocketMemoryPoolSize + SocketAllocatorSize, os::MemoryPageSize);
        constexpr inline size_t UsbRequiredSize      = 2 * UsbDmaBufferSize + UsbIndicationThreadStackSize;
        static_assert(util::IsAligned(UsbDmaBufferSize, RequiredAlignment));

        constexpr inline size_t RequiredSize = std::max(SocketRequiredSize, UsbRequiredSize);
        static_assert(util::IsAligned(RequiredSize, os::MemoryPageSize));

        /* Declare the memory pool. */
        alignas(RequiredAlignment) constinit u8 g_driver_memory[RequiredSize];

        constexpr inline const socket::SystemConfigDefault SocketConfig(g_driver_memory, RequiredSize, SocketAllocatorSize, 2);

    }

    void *GetUsbReceiveBuffer() {
        return g_driver_memory;
    }

    void *GetUsbSendBuffer() {
        return g_driver_memory + UsbDmaBufferSize;
    }

    void *GetUsbIndicationThreadStack() {
        return g_driver_memory + 2 * UsbDmaBufferSize;
    }

    void InitializeSocketApiForSocketDriver() {
        R_ABORT_UNLESS(socket::Initialize(SocketConfig));
    }

}
