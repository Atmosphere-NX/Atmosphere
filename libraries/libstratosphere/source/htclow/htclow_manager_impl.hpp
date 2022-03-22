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
#include "htclow_packet_factory.hpp"
#include "driver/htclow_driver_manager.hpp"
#include "mux/htclow_mux.hpp"
#include "ctrl/htclow_ctrl_packet_factory.hpp"
#include "ctrl/htclow_ctrl_state_machine.hpp"
#include "ctrl/htclow_ctrl_service.hpp"
#include "htclow_worker.hpp"
#include "htclow_listener.hpp"

namespace ams::htclow {

    class HtclowManagerImpl {
        private:
            PacketFactory m_packet_factory;
            driver::DriverManager m_driver_manager;
            mux::Mux m_mux;
            ctrl::HtcctrlPacketFactory m_ctrl_packet_factory;
            ctrl::HtcctrlStateMachine m_ctrl_state_machine;
            ctrl::HtcctrlService m_ctrl_service;
            Worker m_worker;
            Listener m_listener;
            bool m_is_driver_open;
        public:
            HtclowManagerImpl(mem::StandardAllocator *allocator);
            ~HtclowManagerImpl();
        public:
            Result OpenDriver(impl::DriverType driver_type);
            void CloseDriver();

            Result Open(impl::ChannelInternalType channel);
            Result Close(impl::ChannelInternalType channel);

            void Resume();
            void Suspend();

            Result ConnectBegin(u32 *out_task_id, impl::ChannelInternalType channel);
            Result ConnectEnd(impl::ChannelInternalType channel, u32 task_id);

            void Disconnect();

            Result FlushBegin(u32 *out_task_id, impl::ChannelInternalType channel);
            Result FlushEnd(u32 task_id);

            ChannelState GetChannelState(impl::ChannelInternalType channel);
            os::EventType *GetChannelStateEvent(impl::ChannelInternalType channel);

            impl::DriverType GetDriverType();

            os::EventType *GetTaskEvent(u32 task_id);

            void NotifyAsleep();
            void NotifyAwake();

            Result ReceiveBegin(u32 *out_task_id, impl::ChannelInternalType channel, size_t size);
            Result ReceiveEnd(size_t *out, void *dst, size_t dst_size, impl::ChannelInternalType channel, u32 task_id);

            Result SendBegin(u32 *out_task_id, size_t *out, const void *src, size_t src_size, impl::ChannelInternalType channel);
            Result SendEnd(u32 task_id);

            Result WaitReceiveBegin(u32 *out_task_id, impl::ChannelInternalType channel, size_t size);
            Result WaitReceiveEnd(u32 task_id);

            void SetConfig(impl::ChannelInternalType channel, const ChannelConfig &config);

            void SetDebugDriver(driver::IDriver *driver);

            void SetReceiveBuffer(impl::ChannelInternalType channel, void *buf, size_t buf_size);
            void SetSendBuffer(impl::ChannelInternalType channel, void *buf, size_t buf_size);
            void SetSendBufferWithData(impl::ChannelInternalType channel, const void *buf, size_t buf_size);

            Result Shutdown(impl::ChannelInternalType channel);
    };

}
