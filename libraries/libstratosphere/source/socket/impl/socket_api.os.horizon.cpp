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
            socket::impl::SetLastError(Errno::ENoMem);
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
            socket::impl::SetLastError(Errno::ENoMem);
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

                R_ABORT_UNLESS(sm::Initialize());
                R_ABORT_UNLESS(::bsdInitialize(std::addressof(libnx_config), static_cast<u32>(config.GetConcurrencyCountMax()), service_type));
            }

            /* Set the heap generation. */
            g_heap_generation = (g_heap_generation + 1) % MinimumHeapAlignment;

            /* TODO: socket::resolver::EnableResolverCalls()? Not necessary in our case (htc), but consider calling it. */

            g_initialized = true;

            return ResultSuccess();
        }

        ALWAYS_INLINE struct sockaddr *ConvertForLibnx(SockAddr *addr) {
            static_assert(sizeof(SockAddr) == sizeof(struct sockaddr));
            static_assert(alignof(SockAddr) == alignof(struct sockaddr));
            return reinterpret_cast<struct sockaddr *>(addr);
        }

        ALWAYS_INLINE const struct sockaddr *ConvertForLibnx(const SockAddr *addr) {
            static_assert(sizeof(SockAddr) == sizeof(struct sockaddr));
            static_assert(alignof(SockAddr) == alignof(struct sockaddr));
            return reinterpret_cast<const struct sockaddr *>(addr);
        }

        static_assert(std::same_as<SockLenT, socklen_t>);

        Errno TranslateResultToBsdErrorImpl(const Result &result) {
            if (R_SUCCEEDED(result)) {
                return Errno::ESuccess;
            } else if (svc::ResultInvalidCurrentMemory::Includes(result) || svc::ResultOutOfAddressSpace::Includes(result)) {
                return Errno::EFault;
            } else if (sf::hipc::ResultCommunicationError::Includes(result)) {
                return Errno::EL3Hlt;
            } else if (sf::hipc::ResultOutOfResource::Includes(result)) {
                return Errno::EAgain;
            } else {
                R_ABORT_UNLESS(result);
                return static_cast<Errno>(-1);
            }
        }

        ALWAYS_INLINE void TranslateResultToBsdError(Errno &bsd_error, int &result) {
            Errno translate_error = Errno::ESuccess;
            if ((translate_error = TranslateResultToBsdErrorImpl(static_cast<::ams::Result>(::g_bsdResult))) != Errno::ESuccess) {
                bsd_error  = translate_error;
                result     = -1;
            }
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

    ssize_t RecvFrom(s32 desc, void *buffer, size_t buffer_size, MsgFlag flags, SockAddr *out_address, SockLenT *out_addr_len) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Check input. */
        if (buffer_size == 0) {
            return 0;
        } else if (buffer == nullptr) {
            socket::impl::SetLastError(Errno::EInval);
            return -1;
        } else if (buffer_size > std::numeric_limits<u32>::max()) {
            socket::impl::SetLastError(Errno::EFault);
            return -1;
        }

        /* If this is just a normal receive call, perform a normal receive. */
        if (out_address == nullptr || out_addr_len == nullptr || *out_addr_len == 0) {
            return impl::Recv(desc, buffer, buffer_size, flags);
        }

        /* Perform the call. */
        socklen_t length;
        Errno error = Errno::ESuccess;
        int result  = ::bsdRecvFrom(desc, buffer, buffer_size, static_cast<int>(flags), ConvertForLibnx(out_address), std::addressof(length));
        TranslateResultToBsdError(error, result);

        if (result >= 0) {
            *out_addr_len = length;
        } else {
            socket::impl::SetLastError(error);
        }

        return result;
    }

    ssize_t Recv(s32 desc, void *buffer, size_t buffer_size, MsgFlag flags) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Check input. */
        if (buffer_size == 0) {
            return 0;
        } else if (buffer == nullptr) {
            socket::impl::SetLastError(Errno::EInval);
            return -1;
        } else if (buffer_size > std::numeric_limits<u32>::max()) {
            socket::impl::SetLastError(Errno::EFault);
            return -1;
        }

        /* Perform the call. */
        Errno error = Errno::ESuccess;
        int result  = ::bsdRecv(desc, buffer, buffer_size, static_cast<int>(flags));
        TranslateResultToBsdError(error, result);

        if (result < 0) {
            socket::impl::SetLastError(error);
        }

        return result;
    }

    ssize_t SendTo(s32 desc, const void *buffer, size_t buffer_size, MsgFlag flags, const SockAddr *address, SockLenT len) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* If this is a normal send, perform a normal send. */
        if (address == nullptr || len == 0) {
            return impl::Send(desc, buffer, buffer_size, flags);
        }

        /* Check input. */
        if (buffer_size == 0) {
            return 0;
        } else if (buffer == nullptr) {
            socket::impl::SetLastError(Errno::EInval);
            return -1;
        } else if (buffer_size > std::numeric_limits<u32>::max()) {
            socket::impl::SetLastError(Errno::EFault);
            return -1;
        }

        /* Perform the call. */
        Errno error = Errno::ESuccess;
        int result = ::bsdSendTo(desc, buffer, buffer_size, static_cast<int>(flags), ConvertForLibnx(address), len);
        TranslateResultToBsdError(error, result);

        if (result < 0) {
            socket::impl::SetLastError(error);
        }

        return result;
    }

    ssize_t Send(s32 desc, const void *buffer, size_t buffer_size, MsgFlag flags) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Check input. */
        if (buffer_size == 0) {
            return 0;
        } else if (buffer == nullptr) {
            socket::impl::SetLastError(Errno::EInval);
            return -1;
        } else if (buffer_size > std::numeric_limits<u32>::max()) {
            socket::impl::SetLastError(Errno::EFault);
            return -1;
        }

        /* Perform the call. */
        Errno error = Errno::ESuccess;
        int result  = ::bsdSend(desc, buffer, buffer_size, static_cast<int>(flags));
        TranslateResultToBsdError(error, result);

        if (result < 0) {
            socket::impl::SetLastError(error);
        }

        return result;
    }

    s32 Shutdown(s32 desc, ShutdownMethod how) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Perform the call. */
        Errno error = Errno::ESuccess;
        int result  = ::bsdShutdown(desc, static_cast<int>(how));
        TranslateResultToBsdError(error, result);

        if (result < 0) {
            socket::impl::SetLastError(error);
        }

        return result;
    }

    s32 Socket(Family domain, Type type, Protocol protocol) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Perform the call. */
        Errno error = Errno::ESuccess;
        int result  = ::bsdSocket(static_cast<int>(domain), static_cast<int>(type), static_cast<int>(protocol));
        TranslateResultToBsdError(error, result);

        if (result < 0) {
            socket::impl::SetLastError(error);
        }

        return result;
    }

    s32 SocketExempt(Family domain, Type type, Protocol protocol) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Perform the call. */
        Errno error = Errno::ESuccess;
        int result  = ::bsdSocketExempt(static_cast<int>(domain), static_cast<int>(type), static_cast<int>(protocol));
        TranslateResultToBsdError(error, result);

        if (result < 0) {
            socket::impl::SetLastError(error);
        }

        return result;
    }

    s32 Accept(s32 desc, SockAddr *out_address, SockLenT *out_addr_len) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Check input. */
        if (out_address == nullptr && out_addr_len != nullptr && *out_addr_len != 0) {
            socket::impl::SetLastError(Errno::EFault);
            return -1;
        }

        socklen_t addrlen = static_cast<socklen_t>((out_address && out_addr_len) ? *out_addr_len : 0);

        /* Perform the call. */
        Errno error = Errno::ESuccess;
        int result  = ::bsdAccept(desc, ConvertForLibnx(out_address), std::addressof(addrlen));
        TranslateResultToBsdError(error, result);

        if (result >= 0) {
            if (out_addr_len != nullptr) {
                *out_addr_len = addrlen;
            }
        } else {
            socket::impl::SetLastError(error);
        }

        return result;
    }
    s32 Bind(s32 desc, const SockAddr *address, SockLenT len) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Check input. */
        if (address == nullptr || len == 0) {
            socket::impl::SetLastError(Errno::EInval);
            return -1;
        }

        /* Perform the call. */
        Errno error = Errno::ESuccess;
        int result  = ::bsdBind(desc, ConvertForLibnx(address), len);
        TranslateResultToBsdError(error, result);

        if (result < 0) {
            socket::impl::SetLastError(error);
        }

        return result;
    }

    s32 GetSockName(s32 desc, SockAddr *out_address, SockLenT *out_addr_len) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Check input. */
        if (out_address == nullptr || out_addr_len == nullptr || *out_addr_len == 0) {
            socket::impl::SetLastError(Errno::EInval);
            return -1;
        }

        /* Perform the call. */
        socklen_t length;
        Errno error = Errno::ESuccess;
        int result  = ::bsdGetSockName(desc, ConvertForLibnx(out_address), std::addressof(length));
        TranslateResultToBsdError(error, result);

        if (result >= 0) {
            *out_addr_len = length;
        } else {
            socket::impl::SetLastError(error);
        }

        return result;
    }

    s32 SetSockOpt(s32 desc, Level level, Option option_name, const void *option_value, SockLenT option_size) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Check input. */
        if (option_value == nullptr) {
            socket::impl::SetLastError(Errno::EInval);
            return -1;
        }

        /* Perform the call. */
        Errno error = Errno::ESuccess;
        int result  = ::bsdSetSockOpt(desc, static_cast<int>(level), static_cast<int>(option_name), option_value, option_size);
        TranslateResultToBsdError(error, result);

        if (result < 0) {
            socket::impl::SetLastError(error);
        }

        return result;
    }

    s32 Listen(s32 desc, s32 backlog) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Perform the call. */
        Errno error = Errno::ESuccess;
        int result  = ::bsdListen(desc, backlog);
        TranslateResultToBsdError(error, result);

        if (result < 0) {
            socket::impl::SetLastError(error);
        }

        return result;
    }

    s32 Close(s32 desc) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Perform the call. */
        Errno error = Errno::ESuccess;
        int result  = ::bsdClose(desc);
        TranslateResultToBsdError(error, result);

        if (result < 0) {
            socket::impl::SetLastError(error);
        }

        return result;
    }

}
