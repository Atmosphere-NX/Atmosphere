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
#include <stratosphere.hpp>
#include "htclow_worker.hpp"

namespace ams::htclow {

    Worker::Worker(mem::StandardAllocator *allocator, mux::Mux *mux, ctrl::HtcctrlService *ctrl_srv)
        : m_thread_stack_size(4_KB), m_allocator(allocator), m_mux(mux), m_service(ctrl_srv), m_driver(nullptr), m_event(os::EventClearMode_ManualClear), m_7C400()
    {
        /* Allocate stacks. */
        m_receive_thread_stack = m_allocator->Allocate(m_thread_stack_size, os::ThreadStackAlignment);
        m_send_thread_stack    = m_allocator->Allocate(m_thread_stack_size, os::ThreadStackAlignment);
    }

}
