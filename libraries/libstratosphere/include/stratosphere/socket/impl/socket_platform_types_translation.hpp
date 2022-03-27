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
#include <stratosphere/socket/socket_types.hpp>
#include <stratosphere/socket/socket_options.hpp>
#include <stratosphere/socket/socket_constants.hpp>
#include <stratosphere/socket/socket_errno.hpp>

namespace ams::socket::impl {

    #if defined(ATMOSPHERE_OS_WINDOWS)
    class PosixWinSockConverter {
        private:
            struct SocketData {
                SOCKET winsock;
                bool exempt;
                bool shutdown;

                constexpr SocketData() : winsock(static_cast<SOCKET>(INVALID_SOCKET)), exempt(), shutdown() { /* ... */ }
            };
        private:
            os::SdkMutex m_mutex{};
            SocketData m_data[MaxSocketsPerClient]{};
        private:
            static constexpr int GetInitialIndex(SOCKET winsock) {
                /* The lower 2 bits of a winsock are always zero; Nintendo uses the upper bits as a hashmap index into m_data. */
                return (winsock >> 2) % MaxSocketsPerClient;
            }
        public:
            constexpr PosixWinSockConverter() = default;

            s32 AcquirePosixHandle(SOCKET winsock, bool exempt = false);
            s32 GetShutdown(bool &shutdown, s32 posix);
            s32 GetSocketExempt(bool &exempt, s32 posix);
            SOCKET PosixToWinsockSocket(s32 posix);
            void ReleaseAllPosixHandles();
            void ReleasePosixHandle(s32 posix);
            s32 SetShutdown(s32 posix, bool shutdown);
            s32 SetSocketExempt(s32 posix, bool exempt);
            s32 WinsockToPosixSocket(SOCKET winsock);
    };

    s32 MapProtocolValue(Protocol protocol);
    Protocol MapProtocolValue(s32 protocol);

    s32 MapTypeValue(Type type);
    Type MapTypeValue(s32 type);

    s8 MapFamilyValue(Family family);
    Family MapFamilyValue(s8 family);

    s32 MapMsgFlagValue(MsgFlag flag);
    MsgFlag MapMsgFlagValue(s32 flag);

    u32 MapAddrInfoFlagValue(AddrInfoFlag flag);
    AddrInfoFlag MapAddrInfoFlagValue(u32 flag);

    u32 MapShutdownMethodValue(ShutdownMethod how);
    ShutdownMethod MapShutdownMethodValue(u32 how);

    u32 MapFcntlFlagValue(FcntlFlag flag);
    FcntlFlag MapFcntlFlagValue(u32 flag);

    s32 MapLevelValue(Level level);
    Level MapLevelValue(s32 level);

    s32 MapOptionValue(Level level, Option option);
    Option MapOptionValue(s32 level, s32 option);

    s32 MapErrnoValue(Errno error);
    Errno MapErrnoValue(s32 error);

    #endif

    #define AMS_SOCKET_IMPL_DECLARE_CONVERSION(AMS, PLATFORM) \
        void CopyToPlatform(PLATFORM *dst, const AMS *src);   \
        void CopyFromPlatform(AMS *dst, const PLATFORM *src);

    AMS_SOCKET_IMPL_DECLARE_CONVERSION(SockAddrIn, sockaddr_in);
    AMS_SOCKET_IMPL_DECLARE_CONVERSION(TimeVal, timeval);
    AMS_SOCKET_IMPL_DECLARE_CONVERSION(Linger, linger);

    #undef AMS_SOCKET_IMPL_DECLARE_CONVERSION

}
