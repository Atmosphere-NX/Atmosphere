/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "socket_api.hpp"
#include "socket_allocator.hpp"

extern "C" {

#include <switch/services/bsd.h>

}

namespace ams::socket::impl {

    namespace {

        constinit bool g_initialized = false;

        constinit os::SdkMutex g_heap_mutex;

        constinit lmem::HeapHandle g_heap_handle = nullptr;
        constinit int g_heap_generation = -1;

        ALWAYS_INLINE bool IsInitialized() {
            return g_initialized;
        }

    }

    void *Alloc(size_t size) {
        std::scoped_lock lk(g_heap_mutex);

        AMS_ASSERT(g_heap_generation > 0);

        void *ptr = nullptr;

        if (!g_heap_handle) {
            socket::impl::SetLastError(Errno::EOpNotSupp);
        } else if ((ptr = lmem::AllocateFromExpHeap(g_heap_handle, size)) == nullptr) {
            socket::impl::SetLastError(Errno::EOpNotSupp);
        }

        return ptr;
    }

    void *Calloc(size_t num, size_t size) {
        std::scoped_lock lk(g_heap_mutex);

        AMS_ASSERT(g_heap_generation > 0);

        void *ptr = nullptr;

        if (!g_heap_handle) {
            socket::impl::SetLastError(Errno::EOpNotSupp);
        } else if ((ptr = lmem::AllocateFromExpHeap(g_heap_handle, size * num)) == nullptr) {
            socket::impl::SetLastError(Errno::EOpNotSupp);
        } else {
            std::memset(ptr, 0, size * num);
        }

        return ptr;
    }

    void Free(void *ptr) {
        std::scoped_lock lk(g_heap_mutex);

        AMS_ASSERT(g_heap_generation > 0);

        if (!g_heap_handle) {
            socket::impl::SetLastError(Errno::EOpNotSupp);
        } else if (ptr != nullptr) {
            lmem::FreeToExpHeap(g_heap_handle, ptr);
        }
    }

    bool HeapIsAvailable(int generation) {
        std::scoped_lock lk(g_heap_mutex);

        return g_heap_handle && g_heap_generation == generation;
    }

    int GetHeapGeneration() {
        std::scoped_lock lk(g_heap_mutex);

        return g_heap_generation;
    }

    Errno GetLastError() {
        if (AMS_LIKELY(IsInitialized())) {
            return static_cast<Errno>(errno);
        } else {
            return Errno::EInval;
        }
    }

    void SetLastError(Errno err) {
        if (AMS_LIKELY(IsInitialized())) {
            errno = static_cast<int>(err);
        }
    }

    u32 InetHtonl(u32 host) {
        if constexpr (util::IsBigEndian()) {
            return host;
        } else {
            return util::SwapBytes(host);
        }
    }

    u16 InetHtons(u16 host) {
        if constexpr (util::IsBigEndian()) {
            return host;
        } else {
            return util::SwapBytes(host);
        }
    }

    u32 InetNtohl(u32 net) {
        if constexpr (util::IsBigEndian()) {
            return net;
        } else {
            return util::SwapBytes(net);
        }
    }

    u16 InetNtohs(u16 net) {
        if constexpr (util::IsBigEndian()) {
            return net;
        } else {
            return util::SwapBytes(net);
        }
    }

    namespace {

        void InitializeHeapImpl(void *buffer, size_t size) {
            /* NOTE: Nintendo uses both CreateOption_ThreadSafe *and* a global heap mutex. */
            /*       This is unnecessary, and using a single SdkMutex is more performant, since we're not recursive. */
            std::scoped_lock lk(g_heap_mutex);

            g_heap_handle = lmem::CreateExpHeap(buffer, size, lmem::CreateOption_None);
        }

        Result InitializeCommon(const Config &config) {
            /* Check pre-conditions. */
            AMS_ABORT_UNLESS(!IsInitialized());
            AMS_ABORT_UNLESS(config.GetMemoryPool() != nullptr);
            AMS_ABORT_UNLESS(1 <= config.GetConcurrencyCountMax() && config.GetConcurrencyCountMax() <= ConcurrencyLimitMax);
            if (!config.IsSmbpClient()) { AMS_ABORT_UNLESS(config.GetAllocatorPoolSize() < config.GetMemoryPoolSize()); }
            AMS_ABORT_UNLESS(util::IsAligned(config.GetMemoryPoolSize(), os::MemoryPageSize));
            AMS_ABORT_UNLESS(util::IsAligned(config.GetAllocatorPoolSize(), os::MemoryPageSize));
            AMS_ABORT_UNLESS(config.GetAllocatorPoolSize() >= 4_KB);
            if (!config.IsSystemClient()) {
                R_UNLESS(config.GetMemoryPoolSize() >= socket::MinSocketMemoryPoolSize, socket::ResultInsufficientProvidedMemory());
            }

            const size_t transfer_memory_size = config.GetMemoryPoolSize() - config.GetAllocatorPoolSize();
            if (!config.IsSmbpClient()) {
                R_UNLESS(transfer_memory_size >= socket::MinMemHeapAllocatorSize, socket::ResultInsufficientProvidedMemory());
            } else {
                R_UNLESS(config.GetMemoryPoolSize() >= socket::MinimumSharedMbufPoolReservation, socket::ResultInsufficientProvidedMemory());
            }

            /* Initialize the allocator heap. */
            InitializeHeapImpl(static_cast<u8 *>(config.GetMemoryPool()) + transfer_memory_size, config.GetAllocatorPoolSize());

            /* Initialize libnx. */
            {
                const ::BsdInitConfig libnx_config = {
                    .version             = config.GetVersion(),
                    .tmem_buffer         = config.GetMemoryPool(),
                    .tmem_buffer_size    = transfer_memory_size,

                    .tcp_tx_buf_size     = static_cast<u32>(config.GetTcpInitialSendBufferSize()),
                    .tcp_rx_buf_size     = static_cast<u32>(config.GetTcpInitialReceiveBufferSize()),
                    .tcp_tx_buf_max_size = static_cast<u32>(config.GetTcpAutoSendBufferSizeMax()),
                    .tcp_rx_buf_max_size = static_cast<u32>(config.GetTcpAutoReceiveBufferSizeMax()),

                    .udp_tx_buf_size     = static_cast<u32>(config.GetUdpSendBufferSize()),
                    .udp_rx_buf_size     = static_cast<u32>(config.GetUdpReceiveBufferSize()),

                    .sb_efficiency       = static_cast<u32>(config.GetSocketBufferEfficiency()),
                };

                const auto service_type = config.IsSystemClient() ? (1 << 1) : (1 << 0);

                sm::DoWithSession([&] {
                    R_ABORT_UNLESS(::bsdInitialize(std::addressof(libnx_config), static_cast<u32>(config.GetConcurrencyCountMax()), service_type));
                });
            }

            /* Set the heap generation. */
            g_heap_generation = (g_heap_generation + 1) % MinimumHeapAlignment;

            /* TODO: socket::resolver::EnableResolverCalls()? Not necessary in our case (htc), but consider calling it. */

            return ResultSuccess();
        }

    }

    Result Initialize(const Config &config) {
        return InitializeCommon(config);
    }

    Result Finalize() {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* TODO: If we support statistics, kill the statistics thread. */

        /* TODO: socket::resolver::DisableResolverCalls()? */

        /* Finalize libnx. */
        ::bsdExit();

        /* Finalize the heap. */
        lmem::HeapHandle heap_handle;
        {
            std::scoped_lock lk(g_heap_mutex);

            heap_handle   = g_heap_handle;
            g_heap_handle = nullptr;
        }
        lmem::DestroyExpHeap(heap_handle);

        return ResultSuccess();
    }

    Result InitializeAllocatorForInternal(void *buffer, size_t size) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(util::IsAligned(size, os::MemoryPageSize));
        AMS_ABORT_UNLESS(size >= 4_KB);

        InitializeHeapImpl(buffer, size);
        return ResultSuccess();
    }

}
