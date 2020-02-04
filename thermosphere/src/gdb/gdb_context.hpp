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

// Lots of code from:
/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "gdb_defines_internal.hpp"
#include "../transport_interface.h"

namespace ams::hyp::gdb {

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

    typedef enum GDBState
    {
        STATE_DISCONNECTED,
        STATE_CONNECTED,
        STATE_ATTACHED,
        STATE_DETACHING,
    } GDBState;

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

            int m_selectedThreadId = 0;
            int m_selectedThreadIdForContinuing = 0;

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

            char *m_commandData = nullptr;
            char *m_commandEnd = nullptr;
            size_t m_lastSentPacketSize = 0ul;
            char *m_buffer = nullptr;
            char *m_workBuffer = nullptr;

        private:
            void MigrateRxIrq(u32 coreId) const;

            DECLARE_HANDLER(Unsupported);

            // Debug
            DECLARE_VERBOSE_HANDLER(Stopped);
            DECLARE_HANDLER(Detach);
            DECLARE_HANDLER(Kill);
            DECLARE_VERBOSE_HANDLER(CtrlC);
            DECLARE_HANDLER(ContinueOrStepDeprecated);
            DECLARE_VERBOSE_HANDLER(Continue);
            DECLARE_HANDLER(GetStopReason);

        public:
            void Initialize(TransportInterfaceType ifaceType, u32 ifaceId, u32 ifaceFlags);
            void Attach();
            void Detach();

            void Acquire();
            void Release();

            constexpr bool IsAttached() const
            {
                return m_state == State::Attached;
            }
    }
}