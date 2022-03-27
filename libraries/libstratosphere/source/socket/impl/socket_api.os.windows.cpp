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

#include <ws2tcpip.h>

#include <stratosphere/socket/impl/socket_platform_types_translation.hpp>

namespace ams::socket::impl {

    extern PosixWinSockConverter g_posix_winsock_converter;

    namespace {

        constinit util::Atomic<int> g_init_counter = 0;

        ALWAYS_INLINE bool IsInitialized() {
            return g_init_counter > 0;
        }

        class FcntlState {
            private:
                FcntlFlag m_flags[MaxSocketsPerClient]{};
                os::SdkRecursiveMutex m_mutexes[MaxSocketsPerClient]{};
            public:
                constexpr FcntlState() = default;
            public:
                void ClearFlag(int fd, FcntlFlag flag) {
                    std::scoped_lock lk(m_mutexes[fd]);

                    m_flags[fd] &= ~flag;
                }

                void ClearFlags(int fd) {
                    std::scoped_lock lk(m_mutexes[fd]);

                    m_flags[fd] = FcntlFlag::None;
                }

                FcntlFlag GetFlags(int fd) {
                    std::scoped_lock lk(m_mutexes[fd]);

                    return m_flags[fd];
                }

                int GetFlagsInt(int fd) {
                    return static_cast<int>(this->GetFlags(fd));
                }

                os::SdkRecursiveMutex &GetSocketLock(int fd) {
                    return m_mutexes[fd];
                }

                bool IsFlagClear(int fd, FcntlFlag flag) {
                    return !this->IsFlagSet(fd, flag);
                }

                bool IsFlagSet(int fd, FcntlFlag flag) {
                    std::scoped_lock lk(m_mutexes[fd]);

                    return (m_flags[fd] & flag) != static_cast<FcntlFlag>(0);
                }

                bool IsSocketBlocking(int fd) {
                    return !this->IsSocketNonBlocking(fd);
                }

                bool IsSocketNonBlocking(int fd) {
                    return this->IsFlagSet(fd, FcntlFlag::O_NonBlock);
                }

                void SetFlag(int fd, FcntlFlag flag) {
                    std::scoped_lock lk(m_mutexes[fd]);

                    m_flags[fd] |= flag;
                }
        };

        constinit FcntlState g_fcntl_state;

        void TransmuteWsaError() {
            switch (::WSAGetLastError()) {
                case WSAEFAULT:    ::WSASetLastError(WSAEINVAL);      break;
                case WSAENOTSOCK:  ::WSASetLastError(WSAEBADF);       break;
                case WSAETIMEDOUT: ::WSASetLastError(WSAEWOULDBLOCK); break;
            }
        }

        template<std::integral T>
        void TransmuteWsaError(T res) {
            if (static_cast<decltype(SOCKET_ERROR)>(res) == SOCKET_ERROR) {
                TransmuteWsaError();
            }
        }

    }

    #define AMS_SOCKET_IMPL_SCOPED_MAKE_NON_BLOCKING(_cond, _fd)                                                                                           \
        /* If the socket is blocking and we need to make it non-blocking, do so. */                                                                        \
        int nonblock_##__LINE__ = 1;                                                                                                                       \
        bool set_nonblock_##__LINE__ = false;                                                                                                              \
        if (_cond && g_fcntl_state.IsSocketBlocking(_fd)) {                                                                                                \
            if (const auto res = ::ioctlsocket(handle, FIONBIO, reinterpret_cast<u_long *>(std::addressof( nonblock_##__LINE__ ))); res == SOCKET_ERROR) { \
                TransmuteWsaError();                                                                                                                       \
                return res;                                                                                                                                \
            }                                                                                                                                              \
                                                                                                                                                           \
            set_nonblock_##__LINE__ = true;                                                                                                                \
        }                                                                                                                                                  \
                                                                                                                                                           \
        ON_SCOPE_EXIT {                                                                                                                                    \
            /* Preserve last error. */                                                                                                                     \
            const auto last_err = socket::impl::GetLastError();                                                                                            \
            ON_SCOPE_EXIT { socket::impl::SetLastError(last_err); };                                                                                       \
                                                                                                                                                           \
            /* Restore non-blocking state. */                                                                                                              \
            if (set_nonblock_##__LINE__) {                                                                                                                 \
                nonblock_##__LINE__ = 0;                                                                                                                   \
                                                                                                                                                           \
                while (true) {                                                                                                                             \
                    const auto restore_res = ::ioctlsocket(handle, FIONBIO, reinterpret_cast<u_long *>(std::addressof( nonblock_##__LINE__ )));            \
                    TransmuteWsaError(restore_res);                                                                                                        \
                    if (!(restore_res == SOCKET_ERROR && socket::impl::GetLastError() == Errno::EInProgress)) {                                            \
                        break;                                                                                                                             \
                    }                                                                                                                                      \
                                                                                                                                                           \
                    os::SleepThread(TimeSpan::FromMilliSeconds(1));                                                                                        \
                }                                                                                                                                          \
            }                                                                                                                                              \
        }

    #define AMS_SOCKET_IMPL_DO_WITH_TRANSMUTE(expr) ({ const auto res = (expr); TransmuteWsaError(res); res; })


    void *Alloc(size_t size) {
        return ::std::malloc(size);
    }

    void *Calloc(size_t num, size_t size) {
        const size_t total_size = size * num;
        void *buf = Alloc(size);
        if (buf != nullptr) {
            std::memset(buf, 0, total_size);
        }
        return buf;
    }

    void Free(void *ptr) {
        return ::std::free(ptr);
    }

    Errno GetLastError() {
        if (AMS_LIKELY(IsInitialized())) {
            return MapErrnoValue(::WSAGetLastError());
        } else {
            return Errno::EInval;
        }
    }

    void SetLastError(Errno err) {
        if (AMS_LIKELY(IsInitialized())) {
            ::WSASetLastError(MapErrnoValue(err));
        }
    }

    u32 InetHtonl(u32 host) {
        return ::htonl(host);
    }

    u16 InetHtons(u16 host) {
        return ::htons(host);
    }

    u32 InetNtohl(u32 net) {
        return ::ntohl(net);
    }

    u16 InetNtohs(u16 net) {
        return ::ntohs(net);
    }

    Result Initialize(const Config &config) {
        AMS_UNUSED(config);

        /* Increment init counter. */
        ++g_init_counter;

        /* Initialize winsock. */
        WSADATA wsa_data;
        WORD wVersionRequested = MAKEWORD(2, 2);

        const auto res = ::WSAStartup(wVersionRequested, std::addressof(wsa_data));
        AMS_ABORT_UNLESS(res == 0);

        /* Initialize time services. */
        R_ABORT_UNLESS(time::Initialize());

        R_SUCCEED();
    }

    Result Finalize() {
        /* Check pre-conditions. */
        --g_init_counter;
        AMS_ABORT_UNLESS(g_init_counter >= 0);

        /* Cleanup WSA. */
        ::WSACleanup();

        /* Finalize time services. */
        time::Finalize();

        /* Release all posix handles. */
        g_posix_winsock_converter.ReleaseAllPosixHandles();

        R_SUCCEED();
    }

    ssize_t RecvFromInternal(s32 desc, void *buffer, size_t buffer_size, MsgFlag flags, SockAddr *out_address, SockLenT *out_addr_len) {
        /* Convert socket. */
        SOCKET handle = g_posix_winsock_converter.PosixToWinsockSocket(desc);

        /* Check input. */
        if (handle == static_cast<SOCKET>(socket::InvalidSocket)) {
            socket::impl::SetLastError(Errno::EBadf);
            return -1;
        }

        /* Convert the sockaddr. */
        sockaddr sa = {};
        socklen_t addr_len = sizeof(sa);

        /* Perform the call. */
        const auto res = ::recvfrom(handle, static_cast<char *>(buffer), static_cast<int>(buffer_size), MapMsgFlagValue(flags), std::addressof(sa), std::addressof(addr_len));
        if (res == SOCKET_ERROR) {
            if (::WSAGetLastError() == WSAESHUTDOWN) {
                ::WSASetLastError(WSAENETDOWN);
            } else {
                TransmuteWsaError();
            }
        }

        /* Set output. */
        if (out_address != nullptr && out_addr_len != nullptr) {
            if (addr_len > static_cast<socklen_t>(sizeof(*out_address))) {
                addr_len = sizeof(*out_address);
            }

            if (*out_addr_len != 0) {
                if (static_cast<socklen_t>(*out_addr_len) > addr_len) {
                    *out_addr_len = addr_len;
                }

                SockAddr sa_pl = {};
                CopyFromPlatform(reinterpret_cast<SockAddrIn *>(std::addressof(sa_pl)), reinterpret_cast<const sockaddr_in *>(std::addressof(sa)));
                std::memcpy(out_address, std::addressof(sa_pl), *out_addr_len);
            }
        }

        return res;
    }

    ssize_t RecvFrom(s32 desc, void *buffer, size_t buffer_size, MsgFlag flags, SockAddr *out_address, SockLenT *out_addr_len) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Convert socket. */
        SOCKET handle = g_posix_winsock_converter.PosixToWinsockSocket(desc);

        /* If the flags have DontWait set, clear WaitAll. */
        if ((flags & MsgFlag::Msg_DontWait) == MsgFlag::Msg_DontWait) {
            flags &= ~MsgFlag::Msg_WaitAll;
        }

        /* If the flags haev WaitAll set but the socket is non-blocking, clear WaitAll. */
        if ((flags & MsgFlag::Msg_WaitAll) == MsgFlag::Msg_WaitAll && g_fcntl_state.IsSocketNonBlocking(desc)) {
            flags &= ~MsgFlag::Msg_WaitAll;
        }

        /* Check input. */
        if (handle == static_cast<SOCKET>(socket::InvalidSocket)) {
            socket::impl::SetLastError(Errno::EBadf);
            return -1;
        } else if (buffer_size == 0) {
            return 0;
        } else if (buffer == nullptr) {
            socket::impl::SetLastError(Errno::EInval);
            return -1;
        } else if (buffer_size > std::numeric_limits<u32>::max()) {
            socket::impl::SetLastError(Errno::EFault);
            return -1;
        }

        /* Handle blocking vs non-blocking. */
        if ((flags & MsgFlag::Msg_DontWait) == MsgFlag::Msg_DontWait) {
            return RecvFromInternal(desc, buffer, buffer_size, flags, out_address, out_addr_len);
        } else {
            /* Lock the socket. */
            std::scoped_lock lk(g_fcntl_state.GetSocketLock(desc));

            /* Clear don't wait from the flags. */
            flags &= MsgFlag::Msg_DontWait;

            /* If the socket is blocking, we need to make it non-blocking. */
            AMS_SOCKET_IMPL_SCOPED_MAKE_NON_BLOCKING(true, desc);

            /* Do the recv from. */
            return RecvFromInternal(desc, buffer, buffer_size, flags, out_address, out_addr_len);
        }
    }

    ssize_t Recv(s32 desc, void *buffer, size_t buffer_size, MsgFlag flags) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Convert socket. */
        SOCKET handle = g_posix_winsock_converter.PosixToWinsockSocket(desc);

        /* Check input. */
        if (handle == static_cast<SOCKET>(socket::InvalidSocket)) {
            socket::impl::SetLastError(Errno::EBadf);
            return -1;
        } else if (buffer_size == 0) {
            return 0;
        } else if (buffer == nullptr) {
            socket::impl::SetLastError(Errno::EInval);
            return -1;
        } else if (buffer_size > std::numeric_limits<u32>::max()) {
            socket::impl::SetLastError(Errno::EFault);
            return -1;
        }

        /* If the socket is blocking, we need to make it non-blocking. */
        AMS_SOCKET_IMPL_SCOPED_MAKE_NON_BLOCKING(((flags & MsgFlag::Msg_DontWait) == MsgFlag::Msg_DontWait), desc);

        /* Perform the call. */
        return AMS_SOCKET_IMPL_DO_WITH_TRANSMUTE(::recv(handle, static_cast<char *>(buffer), static_cast<int>(buffer_size), MapMsgFlagValue(flags & ~MsgFlag::Msg_DontWait)));
    }

    ssize_t SendTo(s32 desc, const void *buffer, size_t buffer_size, MsgFlag flags, const SockAddr *address, SockLenT len) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Convert socket. */
        SOCKET handle = g_posix_winsock_converter.PosixToWinsockSocket(desc);

        /* Check input. */
        if (handle == static_cast<SOCKET>(socket::InvalidSocket)) {
            socket::impl::SetLastError(Errno::EBadf);
            return -1;
        }

        /* Clear don't wait from flags. */
        flags &= ~MsgFlag::Msg_DontWait;

        /* Convert the sockaddr. */
        sockaddr sa = {};
        socket::impl::CopyToPlatform(reinterpret_cast<sockaddr_in *>(std::addressof(sa)), reinterpret_cast<const SockAddrIn *>(address));

        /* Perform the call. */
        const auto res = ::sendto(handle, static_cast<const char *>(buffer), static_cast<int>(buffer_size), MapMsgFlagValue(flags), address != nullptr ? std::addressof(sa) : nullptr, static_cast<socklen_t>(len));
        if (res == SOCKET_ERROR) {
            if (::WSAGetLastError() == WSAESHUTDOWN) {
                ::WSASetLastError(109);
            } else {
                TransmuteWsaError();
            }
        }

        return res;
    }

    ssize_t Send(s32 desc, const void *buffer, size_t buffer_size, MsgFlag flags) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Convert socket. */
        SOCKET handle = g_posix_winsock_converter.PosixToWinsockSocket(desc);

        /* Check input. */
        if (handle == static_cast<SOCKET>(socket::InvalidSocket)) {
            socket::impl::SetLastError(Errno::EBadf);
            return -1;
        }

        /* Perform the call. */
        return AMS_SOCKET_IMPL_DO_WITH_TRANSMUTE(::send(handle, static_cast<const char *>(buffer), static_cast<int>(buffer_size), MapMsgFlagValue(flags)));
    }

    s32 Shutdown(s32 desc, ShutdownMethod how) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Convert socket. */
        SOCKET handle = g_posix_winsock_converter.PosixToWinsockSocket(desc);

        /* Check input. */
        if (handle == static_cast<SOCKET>(socket::InvalidSocket)) {
            socket::impl::SetLastError(Errno::EBadf);
            return -1;
        }

        /* Perform the call. */
        const auto res = ::shutdown(handle, MapShutdownMethodValue(how));
        g_posix_winsock_converter.SetShutdown(desc, true);
        TransmuteWsaError(res);
        return res;
    }

    s32 Socket(Family domain, Type type, Protocol protocol, bool exempt) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        const auto res = ::socket(MapFamilyValue(domain), MapTypeValue(type), MapProtocolValue(protocol));
        TransmuteWsaError(res);

        s32 posix_socket = -1;
        if (res != static_cast<typename std::remove_cv<decltype(res)>::type>(SOCKET_ERROR)) {
            if (posix_socket = g_posix_winsock_converter.AcquirePosixHandle(res, exempt); posix_socket < 0) {
                /* Preserve last error. */
                const auto last_err = socket::impl::GetLastError();
                ON_SCOPE_EXIT { socket::impl::SetLastError(last_err); };

                /* Close the socket. */
                ::closesocket(res);
            }
        }

        return posix_socket;
    }

    s32 Socket(Family domain, Type type, Protocol protocol) {
        return Socket(domain, type, protocol, false);
    }

    s32 SocketExempt(Family domain, Type type, Protocol protocol) {
        return Socket(domain, type, protocol, true);
    }

    s32 Accept(s32 desc, SockAddr *out_address, SockLenT *out_addr_len) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Convert socket. */
        SOCKET handle = g_posix_winsock_converter.PosixToWinsockSocket(desc);

        /* Check input. */
        if (handle == static_cast<SOCKET>(socket::InvalidSocket)) {
            socket::impl::SetLastError(Errno::EBadf);
            return -1;
        }

        /* Check shutdown. */
        bool is_shutdown = false;
        if (const auto res = g_posix_winsock_converter.GetShutdown(is_shutdown, desc); res == SOCKET_ERROR || (res == 0 && is_shutdown)) {
            socket::impl::SetLastError(Errno::EConnAborted);
            return -1;
        }

        /* Accept. */
        sockaddr sa      = {};
        socklen_t sa_len = sizeof(sa);
        const auto res = ::accept(handle, std::addressof(sa), std::addressof(sa_len));
        if (res == static_cast<typename std::remove_cv<decltype(res)>::type>(SOCKET_ERROR)) {
            if (::WSAGetLastError() == WSAEOPNOTSUPP) {
                ::WSASetLastError(WSAEINVAL);
            } else {
                TransmuteWsaError();
            }
        }

        /* Set output. */
        if (out_address != nullptr && out_addr_len != nullptr) {
            if (sa_len > static_cast<socklen_t>(sizeof(*out_address))) {
                sa_len = sizeof(*out_address);
            }

            if (*out_addr_len != 0) {
                if (static_cast<socklen_t>(*out_addr_len) > sa_len) {
                    *out_addr_len = sa_len;
                }

                SockAddr sa_pl = {};
                CopyFromPlatform(reinterpret_cast<SockAddrIn *>(std::addressof(sa_pl)), reinterpret_cast<const sockaddr_in *>(std::addressof(sa)));
                std::memcpy(out_address, std::addressof(sa_pl), *out_addr_len);
            }

            *out_addr_len = sa_len;
        }

        if (res == static_cast<typename std::remove_cv<decltype(res)>::type>(SOCKET_ERROR)) {
            return res;
        }

        s32 fd = -1;
        bool is_exempt = false;
        if (g_posix_winsock_converter.GetSocketExempt(is_exempt, desc) == 0) {
            fd = g_posix_winsock_converter.AcquirePosixHandle(res, is_exempt);
        }

        if (fd < 0) {
            /* Preserve last error. */
            const auto last_err = socket::impl::GetLastError();
            ON_SCOPE_EXIT { socket::impl::SetLastError(last_err); };

            ::closesocket(res);

            return SOCKET_ERROR;
        }

        return fd;
    }

    s32 Bind(s32 desc, const SockAddr *address, SockLenT len) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Convert socket. */
        SOCKET handle = g_posix_winsock_converter.PosixToWinsockSocket(desc);

        /* Check input. */
        if (handle == static_cast<SOCKET>(socket::InvalidSocket)) {
            socket::impl::SetLastError(Errno::EBadf);
            return -1;
        } else if (address == nullptr) {
            socket::impl::SetLastError(Errno::EInval);
            return -1;
        }

        /* Convert the sockaddr. */
        sockaddr sa = {};
        socket::impl::CopyToPlatform(reinterpret_cast<sockaddr_in *>(std::addressof(sa)), reinterpret_cast<const SockAddrIn *>(address));

        return AMS_SOCKET_IMPL_DO_WITH_TRANSMUTE(::bind(handle, std::addressof(sa), static_cast<socklen_t>(len)));
    }

    s32 Connect(s32 desc, const SockAddr *address, SockLenT len) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Convert socket. */
        SOCKET handle = g_posix_winsock_converter.PosixToWinsockSocket(desc);

        /* Check input. */
        if (handle == static_cast<SOCKET>(socket::InvalidSocket)) {
            socket::impl::SetLastError(Errno::EBadf);
            return -1;
        }

        /* Convert the sockaddr. */
        sockaddr sa = {};
        if (address != nullptr) {
            if (reinterpret_cast<const SockAddrIn *>(address)->sin_port == 0) {
                socket::impl::SetLastError(Errno::EAddrNotAvail);
                return -1;
            }

            socket::impl::CopyToPlatform(reinterpret_cast<sockaddr_in *>(std::addressof(sa)), reinterpret_cast<const SockAddrIn *>(address));
        }

        const auto res = ::connect(handle, address != nullptr ? std::addressof(sa) : nullptr, len);
        if (res == SOCKET_ERROR) {
            const auto wsa_err = ::WSAGetLastError();
            if (wsa_err == WSAEWOULDBLOCK) {
                ::WSASetLastError(WSAEINPROGRESS);
            } else if (wsa_err != WSAETIMEDOUT) {
                TransmuteWsaError();
            }
        }

        return res;
    }

    s32 GetSockName(s32 desc, SockAddr *out_address, SockLenT *out_addr_len) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Convert socket. */
        SOCKET handle = g_posix_winsock_converter.PosixToWinsockSocket(desc);

        /* Check input. */
        if (handle == static_cast<SOCKET>(socket::InvalidSocket)) {
            socket::impl::SetLastError(Errno::EBadf);
            return -1;
        }

        /* We may end up preserving the last wsa error. */
        const auto last_err = ::WSAGetLastError();

        /* Do the call. */
        sockaddr sa = {};

        auto res = ::getsockname(handle, out_address != nullptr ? std::addressof(sa) : nullptr, reinterpret_cast<socklen_t *>(out_addr_len));
        if (res == SOCKET_ERROR) {
            if (::WSAGetLastError() == WSAEINVAL) {
                ::WSASetLastError(last_err);

                sa = {};
                res = 0;
            } else {
                TransmuteWsaError();
            }
        }

        /* Copy out. */
        if (out_address != nullptr) {
            CopyFromPlatform(reinterpret_cast<SockAddrIn *>(out_address), reinterpret_cast<const sockaddr_in *>(std::addressof(sa)));
        }

        return res;
    }

    s32 SetSockOpt(s32 desc, Level level, Option option_name, const void *option_value, SockLenT option_size) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Convert socket. */
        SOCKET handle = g_posix_winsock_converter.PosixToWinsockSocket(desc);

        /* Check input. */
        if (handle == static_cast<SOCKET>(socket::InvalidSocket)) {
            socket::impl::SetLastError(Errno::EBadf);
            return -1;
        }

        union SocketOptionValue {
            linger option_linger;
            DWORD option_timeout_ms;
            DWORD option_exempt;
        };

        SocketOptionValue sockopt_value = {};
        socklen_t option_value_length   = option_size;
        const char *p_option_value      = nullptr;

        switch (option_name) {
            case Option::So_Linger:
            case Option::So_Nn_Linger:
                {
                    if  (option_value_length < static_cast<socklen_t>(sizeof(sockopt_value.option_linger))) {
                        socket::impl::SetLastError(Errno::EInval);
                        return -1;
                    }
                    option_value_length = sizeof(sockopt_value.option_linger);
                    CopyToPlatform(std::addressof(sockopt_value.option_linger), reinterpret_cast<const Linger *>(option_value));
                    p_option_value = reinterpret_cast<const char *>(std::addressof(sockopt_value.option_linger));
                }
                break;
            case Option::So_SndTimeo:
            case Option::So_RcvTimeo:
                {
                    if  (option_value_length < static_cast<socklen_t>(sizeof(sockopt_value.option_timeout_ms))) {
                        socket::impl::SetLastError(Errno::EInval);
                        return -1;
                    }
                    option_value_length = sizeof(sockopt_value.option_timeout_ms);
                    sockopt_value.option_timeout_ms = (reinterpret_cast<const TimeVal *>(option_value)->tv_sec * 1000) + (reinterpret_cast<const TimeVal *>(option_value)->tv_usec / 1000);
                    p_option_value = reinterpret_cast<const char *>(std::addressof(sockopt_value.option_timeout_ms));
                }
                break;
            case Option::So_Nn_Shutdown_Exempt:
                {
                    if (option_value_length < static_cast<socklen_t>(sizeof(sockopt_value.option_exempt))) {
                        socket::impl::SetLastError(Errno::EInval);
                        return -1;
                    }

                    return g_posix_winsock_converter.SetSocketExempt(desc, *reinterpret_cast<const decltype(sockopt_value.option_exempt) *>(option_value) != 0);
                }
                break;
            default:
                p_option_value = reinterpret_cast<const char *>(option_value);
                break;
        }

        return AMS_SOCKET_IMPL_DO_WITH_TRANSMUTE(::setsockopt(handle, MapLevelValue(level), MapOptionValue(level, option_name), p_option_value, option_value_length));
    }

    s32 Listen(s32 desc, s32 backlog) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Convert socket. */
        SOCKET handle = g_posix_winsock_converter.PosixToWinsockSocket(desc);

        /* Check input. */
        if (handle == static_cast<SOCKET>(socket::InvalidSocket)) {
            socket::impl::SetLastError(Errno::EBadf);
            return -1;
        }

        /* Check shutdown. */
        bool is_shutdown = false;
        if (const auto res = g_posix_winsock_converter.GetShutdown(is_shutdown, desc); res == SOCKET_ERROR || (res == 0 && is_shutdown)) {
            socket::impl::SetLastError(Errno::EInval);
            return -1;
        }

        return AMS_SOCKET_IMPL_DO_WITH_TRANSMUTE(::listen(handle, backlog));
    }

    s32 Close(s32 desc) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(IsInitialized());

        /* Check that we can close. */
        static constinit os::SdkMutex s_close_lock;
        SOCKET handle = static_cast<SOCKET>(socket::InvalidSocket);
        {
            std::scoped_lock lk(s_close_lock);

            handle = g_posix_winsock_converter.PosixToWinsockSocket(desc);
            if (handle == static_cast<SOCKET>(socket::InvalidSocket)) {
                return SOCKET_ERROR;
            }

            g_posix_winsock_converter.ReleasePosixHandle(desc);
        }

        /* Do the close. */
        const auto res = ::closesocket(handle);
        g_fcntl_state.ClearFlags(desc);

        return res;
    }

}
