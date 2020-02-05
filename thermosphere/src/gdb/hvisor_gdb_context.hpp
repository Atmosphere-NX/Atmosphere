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

// Lots of code from:
/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "../defines.hpp"
#include "../transport_interface.h"

#include <string_view>

#define _REENT_ONLY
#include <cerrno>

#define DECLARE_HANDLER(name)           int Handle##name()
#define DECLARE_QUERY_HANDLER(name)     DECLARE_HANDLER(Query##name)
#define DECLARE_VERBOSE_HANDLER(name)   DECLARE_HANDLER(Verbose##name)
#define DECLARE_XFER_HANDLER(name)      DECLARE_HANDLER(Xfer##name)

namespace ams::hvisor::gdb {

    struct PackedGdbHioRequest {
        // TODO revamp
        char magic[4]; // "GDB\x00"
        u32 version;

        // Request
        char functionName[16+1];
        char paramFormat[8+1];

        u64 parameters[8];
        size_t stringLengths[8];

        // Return
        s64 retval;
        int gdbErrno;
        bool ctrlC;
    };

    struct DebugEventInfo;

    class Context final {
        NON_COPYABLE(Context);
        NON_MOVEABLE(Context);

        private:
            enum class State {
                Disconnected = 0,
                Connected,
                Attached,
                Detaching
            };

        private:
            // No need for a lock, it's in the transport interface layer...
            TransportInterface *m_transportInterface = nullptr;
            State m_state = State::Disconnected;
            bool m_noAckSent = false;
            bool m_noAck = false;
            bool m_nonStop = false;

            u32 m_attachedCoreList = 0;

            int m_selectedCoreId = 0;
            int m_selectedCoreIdForContinuing = 0;

            u32 m_sentDebugEventCoreList = 0;
            u32 m_acknowledgedDebugEventCoreList = 0;

            bool m_sendOwnDebugEventDisallowed = 0;

            bool m_catchThreadEvents = false;
            bool m_processEnded = false;
            bool m_processExited = false;

            const struct DebugEventInfo *m_lastDebugEvent = nullptr;
            uintptr_t m_currentHioRequestTargetAddr = 0ul;
            PackedGdbHioRequest m_currentHioRequest{};

            size_t m_targetXmlLen = 0;

            char m_commandLetter = '\0';
            std::string_view m_commandData{};
            size_t m_lastSentPacketSize = 0ul;
            char *m_buffer = nullptr;
            char *m_workBuffer = nullptr;

        private:
            void MigrateRxIrq(u32 coreId) const;

            // Comms
            int ReceivePacket();
            int DoSendPacket(size_t len);
            int SendPacket(std::string_view packetData, char hdr = '$');
            int SendFormattedPacket(const char *packetDataFmt, ...);
            int SendHexPacket(const void *packetData, size_t len);
            int SendStreamData(std::string_view streamData, size_t offset, size_t length, bool forceEmptyLast);
            int ReplyOk();
            int ReplyEmpty();
            int ReplyErrno(int no);

            char *GetInPlaceOutputBuffer() const {
                return m_buffer + 1;
            }
        private:
            // Meta
            DECLARE_HANDLER(Unsupported);
            DECLARE_HANDLER(ReadQuery);
            DECLARE_HANDLER(WriteQuery);
            DECLARE_QUERY_HANDLER(Xfer);
            DECLARE_HANDLER(VerboseCommand);

            // General queries
            DECLARE_QUERY_HANDLER(Supported);
            DECLARE_QUERY_HANDLER(StartNoAckMode);
            DECLARE_QUERY_HANDLER(Attached);

            // XML Transfer
            DECLARE_XFER_HANDLER(Features);

            // Resuming features enumeration
            DECLARE_VERBOSE_HANDLER(ContinueSupported);

            // "Threads"
            // Capitalization in "GetTLSAddr" is intended.
            DECLARE_HANDLER(SetThreadId);
            DECLARE_HANDLER(IsThreadAlive);
            DECLARE_QUERY_HANDLER(CurrentThreadId);
            DECLARE_QUERY_HANDLER(fThreadInfo);
            DECLARE_QUERY_HANDLER(sThreadInfo);
            DECLARE_QUERY_HANDLER(ThreadEvents);
            DECLARE_QUERY_HANDLER(ThreadExtraInfo);
            DECLARE_QUERY_HANDLER(GetTLSAddr);

            // Debug
            DECLARE_VERBOSE_HANDLER(Stopped);
            DECLARE_HANDLER(Detach);
            DECLARE_HANDLER(Kill);
            DECLARE_VERBOSE_HANDLER(CtrlC);
            DECLARE_HANDLER(ContinueOrStepDeprecated);
            DECLARE_VERBOSE_HANDLER(Continue);
            DECLARE_HANDLER(GetStopReason);

            // Stop points
            DECLARE_HANDLER(ToggleStopPoint);

            // Memory
            DECLARE_HANDLER(ReadMemory);
            DECLARE_HANDLER(WriteMemory);
            DECLARE_HANDLER(WriteMemoryRaw);
            DECLARE_QUERY_HANDLER(SearchMemory);

            // Registers
            DECLARE_HANDLER(ReadRegisters);
            DECLARE_HANDLER(WriteRegisters);
            DECLARE_HANDLER(ReadRegister);
            DECLARE_HANDLER(WriteRegister);

            // Hio
            DECLARE_HANDLER(HioReply);

            // Custom commands
            DECLARE_QUERY_HANDLER(Rcmd);

        public:
            void Initialize(TransportInterfaceType ifaceType, u32 ifaceId, u32 ifaceFlags);
            void Attach();
            void Detach();

            void Acquire();
            void Release();

            constexpr bool IsAttached() const {
                return m_state == State::Attached;
            }
    };
}

#undef DECLARE_HANDLER
#undef DECLARE_QUERY_HANDLER
#undef DECLARE_VERBOSE_HANDLER
#undef DECLARE_XFER_HANDLER
