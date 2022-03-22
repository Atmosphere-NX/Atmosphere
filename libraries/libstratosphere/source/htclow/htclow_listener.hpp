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
#include "htclow_worker.hpp"

namespace ams::htclow {

    class Listener {
        private:
            u32 m_thread_stack_size;
            mem::StandardAllocator *m_allocator;
            mux::Mux *m_mux;
            ctrl::HtcctrlService *m_service;
            Worker *m_worker;
            os::Event m_event;
            os::ThreadType m_listen_thread;
            void *m_listen_thread_stack;
            driver::IDriver *m_driver;
            bool m_thread_running;
            bool m_cancelled;
        private:
            static void ListenThreadEntry(void *arg) {
                static_cast<Listener *>(arg)->ListenThread();
            }

            void ListenThread();
        public:
            Listener(mem::StandardAllocator *allocator, mux::Mux *mux, ctrl::HtcctrlService *ctrl_srv, Worker *worker);

            void Start(driver::IDriver *driver);
            void Cancel();
            void Wait();
    };

}
