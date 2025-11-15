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
#include "../driver/htc_i_driver.hpp"
#include "htc_htcmisc_rpc_tasks.hpp"

namespace ams::htc::server::rpc {

    class HtcmiscRpcServer {
        private:
            /* TODO: where is this value coming from, again? */
            static constexpr size_t BufferSize = 1_KB;
        private:
            mem::StandardAllocator *m_allocator;
            driver::IDriver *m_driver;
            htclow::ChannelId m_channel_id;
            void *m_receive_thread_stack;
            os::ThreadType m_receive_thread;
            bool m_cancelled;
            bool m_thread_running;
            char m_receive_buffer[BufferSize];
            char m_send_buffer[BufferSize];
            u8 m_driver_receive_buffer[4_KB];
            u8 m_driver_send_buffer[4_KB];
        private:
            static void ReceiveThreadEntry(void *arg) { static_cast<void>(static_cast<HtcmiscRpcServer *>(arg)->ReceiveThread()); }

            Result ReceiveThread();
        public:
            HtcmiscRpcServer(driver::IDriver *driver, htclow::ChannelId channel);
        public:
            void Open();
            void Close();

            Result Start();
            void Cancel();
            void Wait();

            int WaitAny(htclow::ChannelState state, os::EventType *event);
        private:
            Result ProcessSetTargetNameRequest(const char *name, size_t size, u32 task_id);

            Result ReceiveHeader(HtcmiscRpcPacket *header);
            Result ReceiveBody(char *dst, size_t size);
            Result SendRequest(const char *src, size_t size);
    };

}
