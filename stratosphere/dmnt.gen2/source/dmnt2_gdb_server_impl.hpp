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
#pragma once
#include <stratosphere.hpp>
#include "dmnt2_gdb_packet_io.hpp"

namespace ams::dmnt {

    class GdbServerImpl {
        private:
            int m_socket;
            HtcsSession m_session;
            GdbPacketIo m_packet_io;
            char *m_receive_packet{nullptr};
            char *m_reply_packet{nullptr};
            char m_buffer[GdbPacketBufferSize / 2];
            svc::Handle m_debug_handle{svc::InvalidHandle};
        public:
            GdbServerImpl(int socket);
            ~GdbServerImpl();

            void LoopProcess();
        private:
            void ProcessPacket(char *receive, char *reply);

            void SendPacket(bool *out_break, const char *src) { return m_packet_io.SendPacket(out_break, src, std::addressof(m_session)); }
            char *ReceivePacket(bool *out_break, char *dst, size_t size) { return m_packet_io.ReceivePacket(out_break, dst, size, std::addressof(m_session)); }
        private:
            bool HasDebugProcess() const { return m_debug_handle != svc::InvalidHandle; }
            bool Is64Bit() const { return true; /* TODO: Retrieve from debug process info. */ }
        private:
            void H();
            void q();

            void qAttached();
            void qC();
            void qSupported();
            void qXfer();
            void qXferFeaturesRead();
            void qXferOsdataRead();

            void QuestionMark();
    };

}