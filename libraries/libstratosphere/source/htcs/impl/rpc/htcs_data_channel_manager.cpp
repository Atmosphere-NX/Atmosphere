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
#include "htcs_rpc_tasks.hpp"
#include "htcs_data_channel_manager.hpp"
#include "../../../htclow/htclow_channel.hpp"

namespace ams::htcs::impl::rpc {

    Result DataChannelManager::Receive(void *buffer, s64 buffer_size, u32 task_id) {
        /* Check that the buffer size is allowable. */
        R_UNLESS(util::IsIntValueRepresentable<size_t>(buffer_size), htcs::ResultInvalidSize());

        /* Create an htclow channel. */
        htclow::Channel channel(m_htclow_manager);

        /* Open the channel. */
        R_ABORT_UNLESS(channel.Open(std::addressof(m_module), GetReceiveDataChannelId(task_id)));

        /* Ensure that we close the channel, when we're done. */
        ON_SCOPE_EXIT { channel.Close(); };

        /* Set the channel config. */
        constexpr htclow::ChannelConfig BulkReceiveConfig = {
            .flow_control_enabled = false,
            .handshake_enabled    = false,
            .max_packet_size      = 0x3E000,
        };
        channel.SetConfig(BulkReceiveConfig);

        /* Set the receive buffer. */
        channel.SetReceiveBuffer(buffer, buffer_size);

        /* Connect the channel. */
        R_TRY(channel.Connect());

        /* Ensure that we clean up when we're done. */
        ON_SCOPE_EXIT { channel.Shutdown(); };

        /* Notify the receive task. */
        R_TRY(m_rpc_client->Notify<ReceiveTask>(task_id));

        /* Wait to receive the data. */
        R_TRY(channel.WaitReceive(buffer_size));

        return ResultSuccess();
    }

    Result DataChannelManager::Send(const void *buffer, s64 buffer_size, u32 task_id) {
        /* Check that the buffer size is allowable. */
        R_UNLESS(util::IsIntValueRepresentable<size_t>(buffer_size), htcs::ResultInvalidSize());

        /* Create an htclow channel. */
        htclow::Channel channel(m_htclow_manager);

        /* Open the channel. */
        R_ABORT_UNLESS(channel.Open(std::addressof(m_module), GetSendDataChannelId(task_id)));

        /* Ensure that we close the channel, when we're done. */
        ON_SCOPE_EXIT { channel.Close(); };

        /* Set the channel config. */
        constexpr htclow::ChannelConfig BulkSendConfig = {
            .flow_control_enabled = false,
            .handshake_enabled    = false,
            .max_packet_size      = 0x3E000,
        };
        channel.SetConfig(BulkSendConfig);

        /* Set the send buffer. */
        channel.SetSendBufferWithData(buffer, buffer_size);

        /* Connect the channel. */
        R_TRY(channel.Connect());

        /* Ensure that we clean up when we're done. */
        ON_SCOPE_EXIT { channel.Shutdown(); };

        /* Wait to send the data. */
        R_TRY(channel.Flush());

        return ResultSuccess();
    }

}
