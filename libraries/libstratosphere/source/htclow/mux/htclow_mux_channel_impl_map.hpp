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
#include "htclow_mux_channel_impl.hpp"

namespace ams::htclow::mux {

    class ChannelImplMap {
        NON_COPYABLE(ChannelImplMap);
        NON_MOVEABLE(ChannelImplMap);
        public:
            static constexpr int MaxChannelCount = 64;

            using MapType = util::FixedMap<impl::ChannelInternalType, int>;

            static constexpr size_t MapRequiredMemorySize = MapType::GetRequiredMemorySize(MaxChannelCount);
        private:
            PacketFactory *m_packet_factory;
            ctrl::HtcctrlStateMachine *m_state_machine;
            TaskManager *m_task_manager;
            os::Event *m_event;
            u8 m_map_buffer[MapRequiredMemorySize];
            MapType m_map;
            util::TypedStorage<ChannelImpl> m_channel_storage[MaxChannelCount];
            bool m_storage_valid[MaxChannelCount];
        public:
            ChannelImplMap(PacketFactory *pf, ctrl::HtcctrlStateMachine *sm, TaskManager *tm, os::Event *ev);

            ChannelImpl &GetChannelImpl(int index);
            ChannelImpl &GetChannelImpl(impl::ChannelInternalType channel);

            bool Exists(impl::ChannelInternalType channel) const {
                return m_map.find(channel) != m_map.end();
            }

            Result AddChannel(impl::ChannelInternalType channel);
            Result RemoveChannel(impl::ChannelInternalType channel);
        private:
        public:
            MapType &GetMap() {
                return m_map;
            }

            ChannelImpl &operator[](int index) {
                return this->GetChannelImpl(index);
            }
    };

}
