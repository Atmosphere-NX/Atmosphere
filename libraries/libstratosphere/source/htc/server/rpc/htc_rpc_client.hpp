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
#include "../driver/htc_i_driver.hpp"

namespace ams::htc::server::rpc {

    class RpcClient {
        private:
            u64 m_00;
            driver::IDriver *m_driver;
            htclow::ChannelId m_channel_id;
            void *m_receive_thread_stack;
            void *m_send_thread_stack;
            os::ThreadType m_receive_thread;
            os::ThreadType m_send_thread;
            os::SdkMutex &m_mutex;
            /* TODO: m_task_id_free_list */
            /* TODO: m_task_table */
            /* TODO: m_3C0[0x48] */
            /* TODO: m_rpc_task_queue */
            bool m_cancelled;
            bool m_thread_running;
            os::EventType m_5F8_events[0x48];
            os::EventType m_1138_events[0x48];
        public:
            RpcClient(driver::IDriver *driver, htclow::ChannelId channel);
    };

}
