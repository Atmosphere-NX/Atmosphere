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
#include "../../htc/server/driver/htc_i_driver.hpp"
#include "../../htc/server/rpc/htc_rpc_client.hpp"
#include "htcs_service.hpp"

namespace ams::htcs::impl {

    class HtcsMonitor {
        private:
            mem::StandardAllocator *m_allocator;
            htc::server::driver::IDriver *m_driver;
            htc::server::rpc::RpcClient *m_rpc_client;
            HtcsService *m_service;
            void *m_monitor_thread_stack;
            os::ThreadType m_monitor_thread;
            os::SdkMutex m_mutex;
            os::Event m_cancel_event;
            os::Event m_service_availability_event;
            bool m_cancelled;
            bool m_is_service_available;
        private:
            static void ThreadEntry(void *arg) {
                static_cast<HtcsMonitor *>(arg)->ThreadBody();
            }

            void ThreadBody();
        public:
            HtcsMonitor(mem::StandardAllocator *allocator, htc::server::driver::IDriver *drv, htc::server::rpc::RpcClient *rc, HtcsService *srv);
            ~HtcsMonitor();
        public:
            void Start();
            void Cancel();
            void Wait();

            os::EventType *GetServiceAvailabilityEvent() { return m_service_availability_event.GetBase(); }

            bool IsServiceAvailable() {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Get availability. */
                return m_is_service_available;
            }
        private:
            void SetServiceAvailability(bool available) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Set availability. */
                m_is_service_available = available;

                /* Signal availability change. */
                m_service_availability_event.Signal();
            }
    };

}
