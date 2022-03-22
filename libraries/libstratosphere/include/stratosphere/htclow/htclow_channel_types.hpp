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
#include <vapours.hpp>
#include <stratosphere/htclow/htclow_module_types.hpp>

namespace ams::htclow {

    using ChannelId = u16;

    struct ChannelType {
        bool _is_initialized;
        ModuleId _module_id;
        ChannelId _channel_id;
    };

    enum ChannelState {
        ChannelState_Connectable   = 0,
        ChannelState_Unconnectable = 1,
        ChannelState_Connected     = 2,
        ChannelState_Disconnected  = 3,
    };

    struct ChannelConfig {
        bool flow_control_enabled;
        bool handshake_enabled;
        u64 initial_counter_max_data;
        size_t max_packet_size;
    };

    constexpr bool IsStateTransitionAllowed(ChannelState from, ChannelState to) {
        switch (from) {
            case ChannelState_Connectable:
                return to == ChannelState_Unconnectable ||
                       to == ChannelState_Connected ||
                       to == ChannelState_Disconnected;
            case ChannelState_Unconnectable:
                return to == ChannelState_Connectable ||
                       to == ChannelState_Disconnected;
            case ChannelState_Connected:
                return to == ChannelState_Disconnected;
            case ChannelState_Disconnected:
                return to == ChannelState_Disconnected;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

}
