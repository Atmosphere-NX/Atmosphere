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
#include "htc_htclow_driver.hpp"

namespace ams::htc::server::driver {

    namespace {

        constexpr ALWAYS_INLINE htclow::impl::ChannelInternalType GetHtclowChannel(htclow::ChannelId channel_id, htclow::ModuleId module_id) {
            return {
                .channel_id = channel_id,
                .reserved   = 0,
                .module_id  = module_id,
            };
        }

    }

    void HtclowDriver::WaitTask(u32 task_id) {
        os::WaitEvent(m_manager->GetTaskEvent(task_id));
    }

    void HtclowDriver::SetDisconnectionEmulationEnabled(bool en) {
        /* NOTE: Nintendo ignores the input, here. */
        AMS_UNUSED(en);
        m_disconnection_emulation_enabled = false;
    }

    Result HtclowDriver::Open(htclow::ChannelId channel) {
        /* Check the channel/module combination. */
        if (m_module_id == htclow::ModuleId::Htcmisc) {
            AMS_ABORT_UNLESS(channel == HtcmiscClientChannelId );
        } else if (m_module_id == htclow::ModuleId::Htcs) {
            AMS_ABORT_UNLESS(channel == 0);
        } else {
            AMS_ABORT("Unsupported channel");
        }

        return this->Open(channel, m_default_receive_buffer, sizeof(m_default_receive_buffer), m_default_send_buffer, sizeof(m_default_send_buffer));
    }

    Result HtclowDriver::Open(htclow::ChannelId channel, void *receive_buffer, size_t receive_buffer_size, void *send_buffer, size_t send_buffer_size) {
        /* Open the channel. */
        R_TRY(m_manager->Open(GetHtclowChannel(channel, m_module_id)));

        /* Set the send/receive buffers. */
        m_manager->SetReceiveBuffer(GetHtclowChannel(channel, m_module_id), receive_buffer, receive_buffer_size);
        m_manager->SetSendBuffer(GetHtclowChannel(channel, m_module_id), send_buffer, send_buffer_size);

        return ResultSuccess();
    }

    void HtclowDriver::Close(htclow::ChannelId channel) {
        /* Close the channel. */
        const auto result = m_manager->Close(GetHtclowChannel(channel, m_module_id));
        R_ASSERT(result);
    }

    Result HtclowDriver::Connect(htclow::ChannelId channel) {
        /* Check if we should emulate disconnection. */
        R_UNLESS(!m_disconnection_emulation_enabled, htclow::ResultConnectionFailure());

        /* Begin connecting. */
        u32 task_id;
        R_TRY(m_manager->ConnectBegin(std::addressof(task_id), GetHtclowChannel(channel, m_module_id)));

        /* Wait for the task to complete. */
        this->WaitTask(task_id);

        /* Finish connecting. */
        R_TRY(m_manager->ConnectEnd(GetHtclowChannel(channel, m_module_id), task_id));

        return ResultSuccess();
    }

    void HtclowDriver::Shutdown(htclow::ChannelId channel) {
        /* Shut down the channel. */
        m_manager->Shutdown(GetHtclowChannel(channel, m_module_id));
    }

    Result HtclowDriver::Send(s64 *out, const void *src, s64 src_size, htclow::ChannelId channel) {
        /* Check if we should emulate disconnection. */
        R_UNLESS(!m_disconnection_emulation_enabled, htclow::ResultConnectionFailure());

        /* Validate that dst_size is okay. */
        R_UNLESS(util::IsIntValueRepresentable<size_t>(src_size), htclow::ResultOverflow());

        /* Repeatedly send until we're done. */
        size_t cur_send;
        size_t sent;
        for (sent = 0; sent < static_cast<size_t>(src_size); sent += cur_send) {
            /* Begin sending. */
            u32 task_id;
            R_TRY(m_manager->SendBegin(std::addressof(task_id), std::addressof(cur_send), static_cast<const u8 *>(src) + sent, static_cast<size_t>(src_size) - sent, GetHtclowChannel(channel, m_module_id)));

            /* Wait for the task to complete. */
            this->WaitTask(task_id);

            /* Finish sending. */
            R_ABORT_UNLESS(m_manager->SendEnd(task_id));
        }

        /* Set the output sent size. */
        *out = static_cast<s64>(sent);

        return ResultSuccess();
    }

    Result HtclowDriver::ReceiveInternal(size_t *out, void *dst, size_t dst_size, htclow::ChannelId channel, htclow::ReceiveOption option) {
        /* Determine whether we're blocking. */
        const bool blocking = option != htclow::ReceiveOption_NonBlocking;

        /* Begin receiving. */
        u32 task_id;
        R_TRY(m_manager->ReceiveBegin(std::addressof(task_id), GetHtclowChannel(channel, m_module_id), blocking ? 1 : 0));

        /* Wait for the task to complete. */
        this->WaitTask(task_id);

        /* Finish receiving. */
        return m_manager->ReceiveEnd(out, dst, dst_size, GetHtclowChannel(channel, m_module_id), task_id);
    }

    Result HtclowDriver::Receive(s64 *out, void *dst, s64 dst_size, htclow::ChannelId channel, htclow::ReceiveOption option) {
        /* Check if we should emulate disconnection. */
        R_UNLESS(!m_disconnection_emulation_enabled, htclow::ResultConnectionFailure());

        /* Validate that dst_size is okay. */
        R_UNLESS(util::IsIntValueRepresentable<size_t>(dst_size), htclow::ResultOverflow());

        /* Determine the minimum allowable receive size. */
        size_t min_size;
        switch (option) {
            case htclow::ReceiveOption_NonBlocking:    min_size = 0; break;
            case htclow::ReceiveOption_ReceiveAnyData: min_size = 1; break;
            case htclow::ReceiveOption_ReceiveAllData: min_size = dst_size; break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Repeatedly receive. */
        size_t received = 0;
        do {
            size_t cur_received;
            const Result result = this->ReceiveInternal(std::addressof(cur_received), static_cast<u8 *>(dst) + received, static_cast<size_t>(dst_size) - received, channel, option);

            if (R_FAILED(result)) {
                if (htclow::ResultChannelReceiveBufferEmpty::Includes(result)) {
                    R_UNLESS(option != htclow::ReceiveOption_NonBlocking, htclow::ResultNonBlockingReceiveFailed());
                }
                if (htclow::ResultChannelNotExist::Includes(result)) {
                    *out = received;
                }
                return result;
            }

            received += cur_received;
        } while (received < min_size);

        /* Set the output received size. */
        *out = static_cast<s64>(received);

        return ResultSuccess();
    }

    htclow::ChannelState HtclowDriver::GetChannelState(htclow::ChannelId channel) {
        return m_manager->GetChannelState(GetHtclowChannel(channel, m_module_id));
    }

    os::EventType *HtclowDriver::GetChannelStateEvent(htclow::ChannelId channel) {
        return m_manager->GetChannelStateEvent(GetHtclowChannel(channel, m_module_id));
    }

}
