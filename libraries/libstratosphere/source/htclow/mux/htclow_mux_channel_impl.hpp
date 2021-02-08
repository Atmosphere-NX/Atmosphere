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
#include "htclow_mux_task_manager.hpp"
#include "htclow_mux_send_buffer.hpp"

namespace ams::htclow {

    class PacketFactory;

    namespace ctrl {

        class HtcctrlStateMachine;

    }

}

namespace ams::htclow::mux {

    class ChannelImpl {
        private:
            impl::ChannelInternalType m_channel;
            PacketFactory *m_packet_factory;
            ctrl::HtcctrlStateMachine *m_state_machine;
            TaskManager *m_task_manager;
            os::Event *m_event;
            SendBuffer m_send_buffer;
            RingBuffer m_receive_buffer;
            s16 m_version;
            /* TODO: Channel config */
            /* TODO: tracking variables. */
            std::optional<u64> m_108;
            os::Event m_state_change_event;
            ChannelState m_state;
        public:
            ChannelImpl(impl::ChannelInternalType channel, PacketFactory *pf, ctrl::HtcctrlStateMachine *sm, TaskManager *tm, os::Event *ev);

            void SetVersion(s16 version);

            void UpdateState();
        private:
            void ShutdownForce();
            void SetState(ChannelState state);
            void SetStateWithoutCheck(ChannelState state);
    };

}
