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
        /* Forever receive packets. */
        constexpr size_t MaxPacketHeaderSize = std::max(sizeof(PacketHeader), sizeof(ctrl::HtcctrlPacketHeader));
        u8 packet_header_storage[MaxPacketHeaderSize];
        while (true) {
            /* Receive the packet header. */
            R_TRY(m_driver->Receive(packet_header_storage, sizeof(packet_header_storage)));

            /* Check if the packet is a control packet. */
            if (ctrl::HtcctrlPacketHeader *ctrl_header = reinterpret_cast<ctrl::HtcctrlPacketHeader *>(packet_header_storage); ctrl_header->signature == ctrl::HtcctrlSignature) {
                /* Process the packet. */
                R_TRY(this->ProcessReceive(*ctrl_header));
            } else {
                /* Otherwise, we must have a normal packet. */
                PacketHeader *header = reinterpret_cast<PacketHeader *>(packet_header_storage);
                R_UNLESS(header->signature == HtcGen2Signature, htclow::ResultProtocolError());

                /* Process the packet. */
                R_TRY(this->ProcessReceive(*header));
            }
        }
    }

    Result Worker::ProcessReceive(const ctrl::HtcctrlPacketHeader &header) {
        /* Check the header. */
        R_TRY(m_service->CheckReceivedHeader(header));

        /* Receive the body, if we have one. */
        if (header.body_size > 0) {
            R_TRY(m_driver->Receive(m_receive_packet_body, header.body_size));
        }

        /* Process the received packet. */
        m_service->ProcessReceivePacket(header, m_receive_packet_body, header.body_size);

        return ResultSuccess();
    }

    Result Worker::ProcessReceive(const PacketHeader &header) {
        /* Check the header. */
        R_TRY(m_mux->CheckReceivedHeader(header));

        /* Receive the body, if we have one. */
        if (header.body_size > 0) {
            R_TRY(m_driver->Receive(m_receive_packet_body, header.body_size));
        }

        /* Process the received packet. */
        m_mux->ProcessReceivePacket(header, m_receive_packet_body, header.body_size);

        return ResultSuccess();
    }

    Result Worker::ProcessSend() {
        /* Forever process packets. */
        while (true) {
            const auto index = os::WaitAny(m_service->GetSendPacketEvent(), m_mux->GetSendPacketEvent(), m_event.GetBase());
            if (index == 0) {
                /* HtcctrlService packet. */

                /* Clear the packet event. */
                os::ClearEvent(m_service->GetSendPacketEvent());

                /* While we have packets, send them. */
                auto *packet_header = reinterpret_cast<ctrl::HtcctrlPacketHeader *>(m_send_buffer);
                auto *packet_body   = reinterpret_cast<ctrl::HtcctrlPacketBody *>(m_send_buffer + sizeof(*packet_header));
                int body_size;
                while (m_service->QuerySendPacket(packet_header, packet_body, std::addressof(body_size))) {
                    m_service->RemovePacket(*packet_header);
                    R_TRY(m_driver->Send(packet_header, body_size + sizeof(*packet_header)));
                }
            } else if (index == 1) {
                /* Mux packet. */

                /* Clear the packet event. */
                os::ClearEvent(m_mux->GetSendPacketEvent());

                /* While we have packets, send them. */
                auto *packet_header = reinterpret_cast<PacketHeader *>(m_send_buffer);
                auto *packet_body   = reinterpret_cast<PacketBody *>(m_send_buffer + sizeof(*packet_header));
                int body_size;
                while (m_mux->QuerySendPacket(packet_header, packet_body, std::addressof(body_size))) {
                    R_TRY(m_driver->Send(packet_header, body_size + sizeof(*packet_header)));
                    m_mux->RemovePacket(*packet_header);
                }
            } else {
                /* Our event. */

                /* Check if we're cancelled. */
                if (m_cancelled) {
                    return htclow::ResultCancelled();
                }
            }
        }

    }

}
