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
#include "htclow_mux_channel_impl_map.hpp"

namespace ams::htclow::mux {

    ChannelImplMap::ChannelImplMap(PacketFactory *pf, ctrl::HtcctrlStateMachine *sm, TaskManager *tm, os::Event *ev)
        : m_packet_factory(pf), m_state_machine(sm), m_task_manager(tm), m_event(ev), m_map()
    {
        /* Initialize the map. */
        m_map.Initialize(MaxChannelCount, m_map_buffer, sizeof(m_map_buffer));

        /* Set all storages as invalid. */
        for (auto i = 0; i < MaxChannelCount; ++i) {
            m_storage_valid[i] = false;
        }
    }

    ChannelImpl &ChannelImplMap::GetChannelImpl(int index) {
        return GetReference(m_channel_storage[index]);
    }

    ChannelImpl &ChannelImplMap::GetChannelImpl(impl::ChannelInternalType channel) {
        /* Find the channel. */
        auto it = m_map.find(channel);
        AMS_ASSERT(it != m_map.end());

        /* Return the implementation object. */
        return this->GetChannelImpl(it->second);
    }

}
