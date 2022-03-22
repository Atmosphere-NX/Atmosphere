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
#include "htclow_channel.hpp"

namespace ams::htclow {

    Result Channel::Open(const Module *module, ChannelId id) {
        /* Check we're not already initialized. */
        AMS_ASSERT(!module->GetBase()->_is_initialized);

        /* Set our channel type. */
        m_channel = {
            ._is_initialized = true,
            ._module_id      = module->GetBase()->_id,
            ._channel_id     = id,
        };

        /* Open the channel. */
        return m_manager->Open(impl::ConvertChannelType(m_channel));
    }

    void Channel::Close() {
        m_manager->Close(impl::ConvertChannelType(m_channel));
    }

    ChannelState Channel::GetChannelState() {
        return m_manager->GetChannelState(impl::ConvertChannelType(m_channel));
    }

    os::EventType *Channel::GetChannelStateEvent() {
        return m_manager->GetChannelStateEvent(impl::ConvertChannelType(m_channel));
    }

    Result Channel::Connect() {
        const auto channel = impl::ConvertChannelType(m_channel);

        /* Begin the flush. */
        u32 task_id;
        R_TRY(m_manager->ConnectBegin(std::addressof(task_id), channel));

        /* Wait for the task to finish. */
        this->WaitEvent(m_manager->GetTaskEvent(task_id), false);

        /* End the flush. */
        return m_manager->ConnectEnd(channel, task_id);
    }

    Result Channel::Flush() {
        /* Begin the flush. */
        u32 task_id;
        R_TRY(m_manager->FlushBegin(std::addressof(task_id), impl::ConvertChannelType(m_channel)));

        /* Wait for the task to finish. */
        this->WaitEvent(m_manager->GetTaskEvent(task_id), true);

        /* End the flush. */
        return m_manager->FlushEnd(task_id);
    }

    void Channel::Shutdown() {
        m_manager->Shutdown(impl::ConvertChannelType(m_channel));
    }

    Result Channel::Receive(s64 *out, void *dst, s64 size, ReceiveOption option) {
        /* Check pre-conditions. */
        AMS_ABORT_UNLESS(util::IsIntValueRepresentable<size_t>(size));

        /* Determine the minimum allowable receive size. */
        s64 min_size;
        switch (option) {
            case htclow::ReceiveOption_NonBlocking:    min_size = 0; break;
            case htclow::ReceiveOption_ReceiveAnyData: min_size = 1; break;
            case htclow::ReceiveOption_ReceiveAllData: min_size = size; break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Repeatedly receive. */
        s64 received = 0;
        do {
            s64 cur_received;
            const Result result = this->ReceiveInternal(std::addressof(cur_received), static_cast<u8 *>(dst) + received, size - received, option);

            if (R_FAILED(result)) {
                if (htclow::ResultChannelReceiveBufferEmpty::Includes(result)) {
                    R_UNLESS(option != htclow::ReceiveOption_NonBlocking, htclow::ResultNonBlockingReceiveFailed());
                }
                if (htclow::ResultChannelNotExist::Includes(result)) {
                    *out = received;
                }
                return result;
            }

            received += static_cast<size_t>(cur_received);
        } while (received < min_size);

        /* Set output size. */
        AMS_ASSERT(received <= size);
        *out = received;

        return ResultSuccess();
    }

    Result Channel::Send(s64 *out, const void *src, s64 size) {
        /* Check pre-conditions. */
        AMS_ASSERT(util::IsIntValueRepresentable<size_t>(size));

        /* Convert channel. */
        const auto channel = impl::ConvertChannelType(m_channel);

        /* Loop sending. */
        s64 total_sent;
        size_t cur_sent;
        for (total_sent = 0; total_sent < size; total_sent += cur_sent) {
            AMS_ASSERT(util::IsIntValueRepresentable<size_t>(size - total_sent));

            /* Begin the send. */
            u32 task_id;
            const auto begin_result = m_manager->SendBegin(std::addressof(task_id), std::addressof(cur_sent), static_cast<const u8 *>(src) + total_sent, size - total_sent, channel);
            if (R_FAILED(begin_result)) {
                if (total_sent != 0) {
                    break;
                } else {
                    return begin_result;
                }
            }

            /* Wait for the task to finish. */
            this->WaitEvent(m_manager->GetTaskEvent(task_id), true);

            /* Finish the send. */
            R_ABORT_UNLESS(m_manager->SendEnd(task_id));
        }

        /* Set output size. */
        AMS_ASSERT(total_sent <= size);
        *out = total_sent;

        return ResultSuccess();
    }

    void Channel::SetConfig(const ChannelConfig &config) {
        m_manager->SetConfig(impl::ConvertChannelType(m_channel), config);
    }

    void Channel::SetReceiveBuffer(void *buf, size_t size) {
        m_manager->SetReceiveBuffer(impl::ConvertChannelType(m_channel), buf, size);
    }

    void Channel::SetSendBuffer(void *buf, size_t size) {
        m_manager->SetSendBuffer(impl::ConvertChannelType(m_channel), buf, size);
    }

    void Channel::SetSendBufferWithData(const void *buf, size_t size) {
        m_manager->SetSendBufferWithData(impl::ConvertChannelType(m_channel), buf, size);
    }

    Result Channel::WaitReceive(s64 size) {
        /* Check pre-conditions. */
        AMS_ASSERT(util::IsIntValueRepresentable<size_t>(size));

        return this->WaitReceiveInternal(size, nullptr);
    }

    Result Channel::WaitReceive(s64 size, os::EventType *event) {
        /* Check pre-conditions. */
        AMS_ASSERT(util::IsIntValueRepresentable<size_t>(size));
        AMS_ASSERT(event != nullptr);

        return this->WaitReceiveInternal(size, event);
    }

    void Channel::WaitEvent(os::EventType *event, bool) {
        return os::WaitEvent(event);
    }

    Result Channel::ReceiveInternal(s64 *out, void *dst, s64 size, ReceiveOption option) {
        const auto channel  = impl::ConvertChannelType(m_channel);
        const bool blocking = option != ReceiveOption_NonBlocking;

        /* Begin the receive. */
        u32 task_id;
        R_TRY(m_manager->ReceiveBegin(std::addressof(task_id), channel, blocking ? 1 : 0));

        /* Wait for the task to finish. */
        this->WaitEvent(m_manager->GetTaskEvent(task_id), false);

        /* Receive the data. */
        size_t received;
        AMS_ASSERT(util::IsIntValueRepresentable<size_t>(size));
        R_TRY(m_manager->ReceiveEnd(std::addressof(received), dst, static_cast<size_t>(size), channel, task_id));

        /* Set the output size. */
        AMS_ASSERT(util::IsIntValueRepresentable<s64>(received));
        *out = static_cast<s64>(received);

        return ResultSuccess();
    }

    Result Channel::WaitReceiveInternal(s64 size, os::EventType *event) {
        const auto channel = impl::ConvertChannelType(m_channel);

        /* Begin the wait. */
        u32 task_id;
        R_TRY(m_manager->WaitReceiveBegin(std::addressof(task_id), channel, size));


        /* Perform the wait. */
        if (event != nullptr) {
            if (os::WaitAny(event, m_manager->GetTaskEvent(task_id)) == 0) {
                m_manager->WaitReceiveEnd(task_id);
                return htclow::ResultChannelWaitCancelled();
            }
        } else {
            this->WaitEvent(m_manager->GetTaskEvent(task_id), false);
        }

        /* End the wait. */
        return m_manager->WaitReceiveEnd(task_id);
    }

}
