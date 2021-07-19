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
            enum class State {
                Initial,
                Running,
                Exited,
                Destroyed,
            };

            enum GdbSignal {
                GdbSignal_Signal0             =  0,
                GdbSignal_Interrupt           =  2,
                GdbSignal_IllegalInstruction  =  4,
                GdbSignal_BreakpointTrap      =  5,
                GdbSignal_EmulationTrap       =  7,
                GdbSignal_ArithmeticException =  8,
                GdbSignal_Killed              =  9,
                GdbSignal_BusError            = 10,
                GdbSignal_SegmentationFault   = 11,
                GdbSignal_BadSystemCall       = 12,
            };
        private:
            int m_socket;
            HtcsSession m_session;
            GdbPacketIo m_packet_io;
            char *m_receive_packet{nullptr};
            char *m_reply_packet{nullptr};
            char m_buffer[GdbPacketBufferSize / 2];
            bool m_killed{false};
            svc::Handle m_debug_handle{svc::InvalidHandle};
            os::ThreadType m_events_thread;
            State m_state;
            bool m_attached{false};
            u64 m_thread_id{0};
            os::ProcessId m_process_id{os::InvalidProcessId};
            os::ProcessId m_target_process_id{os::InvalidProcessId};
            svc::DebugInfoCreateProcess m_create_process_info;
            os::Event m_event;
        public:
            GdbServerImpl(int socket, void *thread_stack, size_t stack_size);
            ~GdbServerImpl();

            void LoopProcess();
        private:
            void ProcessPacket(char *receive, char *reply);

            void SendPacket(bool *out_break, const char *src) { return m_packet_io.SendPacket(out_break, src, std::addressof(m_session)); }
            char *ReceivePacket(bool *out_break, char *dst, size_t size) { return m_packet_io.ReceivePacket(out_break, dst, size, std::addressof(m_session)); }
        private:
            bool HasDebugProcess() const { return m_debug_handle != svc::InvalidHandle; }
            bool Is64Bit() const { return (m_create_process_info.flags & svc::CreateProcessFlag_Is64Bit) == svc::CreateProcessFlag_Is64Bit; }
            bool Is64BitAddressSpace() const { return (m_create_process_info.flags & svc::CreateProcessFlag_AddressSpaceMask) == svc::CreateProcessFlag_AddressSpace64Bit; }
        private:
            static void DebugEventsThreadEntry(void *arg) { static_cast<GdbServerImpl *>(arg)->DebugEventsThread(); }
            void DebugEventsThread();
            void ProcessDebugEvents();
            void SetStopReplyPacket(GdbSignal signal);
        private:
            void H();
            void Hg();

            bool g();

            void m();

            void v();

            void vAttach();

            void q();

            void qAttached();
            void qC();
            void qSupported();
            void qXfer();
            void qXferFeaturesRead();
            void qXferOsdataRead();
            bool qXferThreadsRead();

            void QuestionMark();
    };

}