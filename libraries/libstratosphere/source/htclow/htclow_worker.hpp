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
#include "driver/htclow_i_driver.hpp"
#include "ctrl/htclow_ctrl_service.hpp"
#include "mux/htclow_mux.hpp"

namespace ams::htclow {

    class Worker {
        private:
            u32 m_thread_stack_size;
            u8 m_04[0x7C024]; /* TODO... not knowing what an almost 128 KB field is is embarassing. */
            mem::StandardAllocator *m_allocator;
            mux::Mux *m_mux;
            ctrl::HtcctrlService *m_service;
            driver::IDriver *m_driver;
            os::ThreadType m_receive_thread;
            os::ThreadType m_send_thread;
            os::EventType m_event;
            void *m_receive_thread_stack;
            void *m_send_thread_stack;
            u8 m_7C400;
        public:
            Worker(mem::StandardAllocator *allocator, mux::Mux *mux, ctrl::HtcctrlService *ctrl_srv);
    };

}
