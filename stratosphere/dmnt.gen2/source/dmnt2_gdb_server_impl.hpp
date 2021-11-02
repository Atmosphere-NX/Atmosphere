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
#include <stratosphere.hpp>
#include "dmnt2_gdb_packet_io.hpp"
#include "dmnt2_gdb_signal.hpp"
#include "dmnt2_debug_process.hpp"

namespace ams::dmnt {

    class GdbServerImpl {
        private:
            enum class State {
                Initial,
                Running,
                Exited,
                Destroyed,
            };
        private:
            int m_socket;
            TransportSession m_session;
            GdbPacketIo m_packet_io;
            char *m_receive_packet{nullptr};
            char *m_reply_cur{nullptr};
            char *m_reply_end{nullptr};
            char m_buffer[GdbPacketBufferSize / 2];
            bool m_killed{false};
            os::ThreadType m_events_thread;
            State m_state;
            DebugProcess m_debug_process;
            os::ProcessId m_process_id{os::InvalidProcessId};
            os::Event m_event;
            os::ProcessId m_wait_process_id{os::InvalidProcessId};
        public:
            GdbServerImpl(int socket, void *thread_stack, size_t stack_size);
            ~GdbServerImpl();

            void LoopProcess();
        private:
            void ProcessPacket(char *receive, char *reply);

            void SendPacket(bool *out_break, const char *src) { return m_packet_io.SendPacket(out_break, src, std::addressof(m_session)); }
            char *ReceivePacket(bool *out_break, char *dst, size_t size) { return m_packet_io.ReceivePacket(out_break, dst, size, std::addressof(m_session)); }
        private:
            bool HasDebugProcess() const { return m_debug_process.IsValid(); }
            bool Is64Bit() const { return m_debug_process.Is64Bit(); }
            bool Is64BitAddressSpace() const { return m_debug_process.Is64BitAddressSpace(); }
        private:
            static void DebugEventsThreadEntry(void *arg) { static_cast<GdbServerImpl *>(arg)->DebugEventsThread(); }
            void DebugEventsThread();
            void ProcessDebugEvents();
            void AppendStopReplyPacket(GdbSignal signal);
        private:
            void D();

            void G();

            void H();
            void Hg();

            void M();

            void P();

            void Q();

            void T();

            void Z();

            void c();

            bool g();

            void k();

            void m();

            void p();

            void v();

            void vAttach();
            void vCont();

            void q();

            void qAttached();
            void qC();
            void qRcmd();
            void qSupported();
            void qXfer();
            void qXferFeaturesRead();
            void qXferLibrariesRead();
            void qXferOsdataRead();
            bool qXferThreadsRead();

            void z();

            void QuestionMark();
        private:
            Result ParseVCont(char * const token, u64 *thread_ids, u8 *continue_modes, s32 num_threads, DebugProcess::ContinueMode &default_continue_mode);
    };

}