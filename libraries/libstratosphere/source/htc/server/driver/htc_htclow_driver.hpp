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
#include "../../../htclow/htclow_manager.hpp"
#include "htc_i_driver.hpp"

namespace ams::htc::server::driver {

    class HtclowDriver final : public IDriver {
        private:
            static constexpr size_t DefaultBufferSize = 128_KB;
        private:
            u8 m_default_receive_buffer[DefaultBufferSize];
            u8 m_default_send_buffer[DefaultBufferSize];
            htclow::HtclowManager *m_manager;
            bool m_disconnection_emulation_enabled;
            htclow::ModuleId m_module_id;
        public:
            HtclowDriver(htclow::HtclowManager *manager, htclow::ModuleId module_id) : m_manager(manager), m_disconnection_emulation_enabled(false), m_module_id(module_id) { /* ... */ }
        private:
            void WaitTask(u32 task_id);
            Result ReceiveInternal(size_t *out, void *dst, size_t dst_size, htclow::ChannelId channel, htclow::ReceiveOption option);
        public:
            virtual void SetDisconnectionEmulationEnabled(bool en) override;
            virtual Result Open(htclow::ChannelId channel) override;
            virtual Result Open(htclow::ChannelId channel, void *receive_buffer, size_t receive_buffer_size, void *send_buffer, size_t send_buffer_size) override;
            virtual void Close(htclow::ChannelId channel) override;
            virtual Result Connect(htclow::ChannelId channel) override;
            virtual void Shutdown(htclow::ChannelId channel) override;
            virtual Result Send(s64 *out, const void *src, s64 src_size, htclow::ChannelId channel) override;
            virtual Result Receive(s64 *out, void *dst, s64 dst_size, htclow::ChannelId channel, htclow::ReceiveOption option) override;
            virtual htclow::ChannelState GetChannelState(htclow::ChannelId channel) override;
            virtual os::EventType *GetChannelStateEvent(htclow::ChannelId channel) override;
    };

}
