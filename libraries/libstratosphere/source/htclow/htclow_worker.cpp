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

    namespace {

        constexpr inline size_t ThreadStackSize = 4_KB;

    }

    Worker::Worker(mem::StandardAllocator *allocator, mux::Mux *mux, ctrl::HtcctrlService *ctrl_srv)
        : m_thread_stack_size(ThreadStackSize), m_allocator(allocator), m_mux(mux), m_service(ctrl_srv), m_driver(nullptr), m_event(os::EventClearMode_ManualClear), m_cancelled(false)
    {
        /* Allocate stacks. */
        m_receive_thread_stack = m_allocator->Allocate(m_thread_stack_size, os::ThreadStackAlignment);
        m_send_thread_stack    = m_allocator->Allocate(m_thread_stack_size, os::ThreadStackAlignment);
    }

    void Worker::SetDriver(driver::IDriver *driver) {
        m_driver = driver;
    }

    void Worker::Start() {
        /* Clear our cancelled state. */
        m_cancelled = false;

        /* Clear our event. */
        m_event.Clear();

        /* Create our threads. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_receive_thread), ReceiveThreadEntry, this, m_receive_thread_stack, ThreadStackSize, AMS_GET_SYSTEM_THREAD_PRIORITY(htc, HtclowReceive)));
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_send_thread),    SendThreadEntry,    this, m_send_thread_stack,    ThreadStackSize, AMS_GET_SYSTEM_THREAD_PRIORITY(htc, HtclowSend)));

        /* Set thread names. */
        os::SetThreadNamePointer(std::addressof(m_receive_thread), AMS_GET_SYSTEM_THREAD_NAME(htc, HtclowReceive));
        os::SetThreadNamePointer(std::addressof(m_send_thread),    AMS_GET_SYSTEM_THREAD_NAME(htc, HtclowSend));

        /* Start our threads. */
        os::StartThread(std::addressof(m_receive_thread));
        os::StartThread(std::addressof(m_send_thread));
    }

    void Worker::Wait() {
        os::WaitThread(std::addressof(m_receive_thread));
        os::WaitThread(std::addressof(m_send_thread));
        os::DestroyThread(std::addressof(m_receive_thread));
        os::DestroyThread(std::addressof(m_send_thread));
    }

    void Worker::ReceiveThread() {
        this->ProcessReceive();
        m_driver->CancelSendReceive();
        this->Cancel();
    }

    void Worker::SendThread() {
        this->ProcessSend();
        m_driver->CancelSendReceive();
        this->Cancel();
    }

    void Worker::Cancel() {
        /* Set ourselves as cancelled, and signal. */
        m_cancelled = true;
        m_event.Signal();
    }

    Result Worker::ProcessReceive() {
        /* TODO */
        AMS_ABORT("Worker::ProcessReceive");
    }

    Result Worker::ProcessSend() {
        /* TODO */
        AMS_ABORT("Worker::ProcessSend");
    }

}
