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
#include "htc_htcmisc_rpc_server.hpp"

namespace ams::htc::server::rpc {

    namespace {

        constexpr inline size_t ReceiveThreadStackSize = os::MemoryPageSize;

        alignas(os::ThreadStackAlignment) constinit u8 g_receive_thread_stack[ReceiveThreadStackSize];

    }

    HtcmiscRpcServer::HtcmiscRpcServer(driver::IDriver *driver, htclow::ChannelId channel)
        : m_allocator(nullptr),
          m_driver(driver),
          m_channel_id(channel),
          m_receive_thread_stack(g_receive_thread_stack),
          m_cancelled(false),
          m_thread_running(false)
    {
        /* ... */
    }

    void HtcmiscRpcServer::Open() {
        R_ABORT_UNLESS(m_driver->Open(m_channel_id, m_driver_receive_buffer, sizeof(m_driver_receive_buffer), m_driver_send_buffer, sizeof(m_driver_send_buffer)));
    }

    void HtcmiscRpcServer::Close() {
        m_driver->Close(m_channel_id);
    }

    Result HtcmiscRpcServer::Start() {
        /* Connect. */
        R_TRY(m_driver->Connect(m_channel_id));

        /* Create our thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_receive_thread), ReceiveThreadEntry, this, m_receive_thread_stack, ReceiveThreadStackSize, AMS_GET_SYSTEM_THREAD_PRIORITY(htc, HtcmiscReceive)));

        /* Set thread name pointer. */
        os::SetThreadNamePointer(std::addressof(m_receive_thread), AMS_GET_SYSTEM_THREAD_NAME(htc, HtcmiscReceive));

        /* Start thread. */
        os::StartThread(std::addressof(m_receive_thread));

        /* Set initial state. */
        m_cancelled      = false;
        m_thread_running = true;

        return ResultSuccess();
    }

    void HtcmiscRpcServer::Cancel() {
        /* Set cancelled. */
        m_cancelled = true;
    }

    void HtcmiscRpcServer::Wait() {
        /* Wait for thread to not be running. */
        if (m_thread_running) {
            os::WaitThread(std::addressof(m_receive_thread));
            os::DestroyThread(std::addressof(m_receive_thread));
        }
        m_thread_running = false;
    }

    int HtcmiscRpcServer::WaitAny(htclow::ChannelState state, os::EventType *event) {
        /* Check if we're already signaled. */
        if (os::TryWaitEvent(event)) {
            return 1;
        }

        /* Wait. */
        while (m_driver->GetChannelState(m_channel_id) != state) {
            const auto idx = os::WaitAny(m_driver->GetChannelStateEvent(m_channel_id), event);
            if (idx != 0) {
                return idx;
            }

            /* Clear the channel state event. */
            os::ClearEvent(m_driver->GetChannelStateEvent(m_channel_id));
        }

        return 0;
    }

    Result HtcmiscRpcServer::ReceiveThread() {
        /* Loop forever. */
        auto *header = reinterpret_cast<HtcmiscRpcPacket *>(m_receive_buffer);
        while (true) {
            /* Try to receive a packet header. */
            R_TRY(this->ReceiveHeader(header));

            /* Track how much we've received. */
            size_t received = sizeof(*header);

            /* If the packet has one, receive its body. */
            if (header->body_size > 0) {
                /* Sanity check the body size. */
                AMS_ABORT_UNLESS(util::IsIntValueRepresentable<size_t>(header->body_size));
                AMS_ABORT_UNLESS(static_cast<size_t>(header->body_size) <= sizeof(m_receive_buffer) - received);

                /* Receive the body. */
                R_TRY(this->ReceiveBody(header->data, header->body_size));

                /* Note that we received the body. */
                received += header->body_size;
            }

            /* Check that the packet is a request packet. */
            R_UNLESS(header->category == HtcmiscPacketCategory::Request, htc::ResultInvalidCategory());

            /* Handle specific requests. */
            if (header->type == HtcmiscPacketType::SetTargetName) {
                R_TRY(this->ProcessSetTargetNameRequest(header->data, header->body_size, header->task_id));
            }
        }
    }

    Result HtcmiscRpcServer::ProcessSetTargetNameRequest(const char *name, size_t size, u32 task_id) {
        /* TODO: we need to use settings::fwdbg::SetSettingsItemValue here, but this will require ams support for set:fd re-enable? */
        /* Needs some thought. */
        AMS_UNUSED(name, size, task_id);
        AMS_ABORT("HtcmiscRpcServer::ProcessSetTargetNameRequest");
    }

    Result HtcmiscRpcServer::ReceiveHeader(HtcmiscRpcPacket *header) {
        /* Receive. */
        s64 received;
        R_TRY(m_driver->Receive(std::addressof(received), reinterpret_cast<char *>(header), sizeof(*header), m_channel_id, htclow::ReceiveOption_ReceiveAllData));

        /* Check size. */
        R_UNLESS(static_cast<size_t>(received) == sizeof(*header), htc::ResultInvalidSize());

        return ResultSuccess();
    }

    Result HtcmiscRpcServer::ReceiveBody(char *dst, size_t size) {
        /* Receive. */
        s64 received;
        R_TRY(m_driver->Receive(std::addressof(received), dst, size, m_channel_id, htclow::ReceiveOption_ReceiveAllData));

        /* Check size. */
        R_UNLESS(static_cast<size_t>(received) == size, htc::ResultInvalidSize());

        return ResultSuccess();
    }

    Result HtcmiscRpcServer::SendRequest(const char *src, size_t size) {
        /* Sanity check our size. */
        AMS_ASSERT(util::IsIntValueRepresentable<s64>(size));

        /* Send the data. */
        s64 sent;
        R_TRY(m_driver->Send(std::addressof(sent), src, static_cast<s64>(size), m_channel_id));

        /* Check that we sent the right amount. */
        R_UNLESS(sent == static_cast<s64>(size), htc::ResultInvalidSize());

        return ResultSuccess();
    }

}
