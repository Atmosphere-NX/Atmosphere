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
#include "htclow_ctrl_state_machine.hpp"

namespace ams::htclow::ctrl {

    namespace {

        constexpr const impl::ChannelInternalType ServiceChannels[] = {
            {
                .channel_id = 0,
                .module_id  = static_cast<ModuleId>(1),
            },
            {
                .channel_id = 1,
                .module_id  = static_cast<ModuleId>(3),
            },
            {
                .channel_id = 2,
                .module_id  = static_cast<ModuleId>(3),
            },
            {
                .channel_id = 0,
                .module_id  = static_cast<ModuleId>(4),
            },
        };

    }

    HtcctrlStateMachine::HtcctrlStateMachine() : m_map(), m_state(HtcctrlState_Eleven), m_prev_state(HtcctrlState_Eleven), m_mutex() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our map. */
        m_map.Initialize(MaxChannelCount, m_map_buffer, sizeof(m_map_buffer));

        /* Insert each service channel the map. */
        for (const auto &channel : ServiceChannels) {
            m_map.insert(std::make_pair<impl::ChannelInternalType, ServiceChannelState>(impl::ChannelInternalType{channel}, ServiceChannelState{}));
        }
    }

}
