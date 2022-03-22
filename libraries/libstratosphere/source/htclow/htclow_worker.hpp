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
#include "driver/htclow_i_driver.hpp"
#include "ctrl/htclow_ctrl_service.hpp"
#include "mux/htclow_mux.hpp"

namespace ams::htclow {

    class Worker {
        private:
            static_assert(sizeof(ctrl::HtcctrlPacketHeader) <= sizeof(PacketHeader));
            static_assert(sizeof(ctrl::HtcctrlPacketBody) <= sizeof(PacketBody));
        private:
            u32 m_thread_stack_size;
            u8 m_send_buffer[sizeof(PacketHeader) + sizeof(PacketBody)];
            u8 m_receive_packet_body[sizeof(PacketBody)];
            mem::StandardAllocator *m_allocator;
            mux::Mux *m_mux;
            ctrl::HtcctrlService *m_service;
            driver::IDriver *m_driver;
            os::ThreadType m_receive_thread;
            os::ThreadType m_send_thread;
            os::Event m_event;
            void *m_receive_thread_stack;
            void *m_send_thread_stack;
            bool m_cancelled;
        private:
            static void ReceiveThreadEntry(void *arg) {
                static_cast<Worker *>(arg)->ReceiveThread();
            }

            static void SendThreadEntry(void *arg) {
                static_cast<Worker *>(arg)->SendThread();
            }

            void ReceiveThread();
            void SendThread();
        public:
            Worker(mem::StandardAllocator *allocator, mux::Mux *mux, ctrl::HtcctrlService *ctrl_srv);

            void SetDriver(driver::IDriver *driver);

            void Start();
            void Cancel();
            void Wait();
        private:
            Result ProcessReceive();
            Result ProcessSend();

            Result ProcessReceive(const ctrl::HtcctrlPacketHeader &header);
            Result ProcessReceive(const PacketHeader &header);
    };

}
