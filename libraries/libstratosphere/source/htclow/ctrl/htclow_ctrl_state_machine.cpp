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

        /* TODO: Real module id names */
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

    HtcctrlStateMachine::HtcctrlStateMachine() : m_map(), m_state(HtcctrlState_DriverDisconnected), m_prev_state(HtcctrlState_DriverDisconnected), m_mutex() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our map. */
        m_map.Initialize(MaxChannelCount, m_map_buffer, sizeof(m_map_buffer));

        /* Insert each service channel the map. */
        for (const auto &channel : ServiceChannels) {
            m_map.insert(std::make_pair<impl::ChannelInternalType, ServiceChannelState>(impl::ChannelInternalType{channel}, ServiceChannelState{}));
        }
    }

    HtcctrlState HtcctrlStateMachine::GetHtcctrlState() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        return m_state;
    }

    Result HtcctrlStateMachine::SetHtcctrlState(bool *out_transitioned, HtcctrlState state) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that the transition is allowed. */
        R_UNLESS(ctrl::IsStateTransitionAllowed(m_state, state), htclow::ResultStateTransitionNotAllowed());

        /* Get the state pre-transition. */
        const auto old_state = m_state;

        /* Set the state. */
        this->SetStateWithoutCheckInternal(state);

        /* Note whether we transitioned. */
        *out_transitioned = state != old_state;
        return ResultSuccess();
    }

    bool HtcctrlStateMachine::IsConnected() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        return ctrl::IsConnected(m_state);
    }

    bool HtcctrlStateMachine::IsReadied() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        return ctrl::IsReadied(m_state);
    }

    bool HtcctrlStateMachine::IsUnconnectable() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        return !ctrl::IsConnected(m_state);
    }

    bool HtcctrlStateMachine::IsDisconnected() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        return ctrl::IsDisconnected(m_state);
    }

    bool HtcctrlStateMachine::IsSleeping() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        return ctrl::IsSleeping(m_state);
    }

    bool HtcctrlStateMachine::IsConnectedStatusChanged() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        return ctrl::IsConnected(m_prev_state) ^ ctrl::IsConnected(m_state);
    }

    bool HtcctrlStateMachine::IsSleepingStatusChanged() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        return ctrl::IsSleeping(m_prev_state) ^ ctrl::IsSleeping(m_state);
    }

    void HtcctrlStateMachine::SetStateWithoutCheckInternal(HtcctrlState state) {
        if (m_state != state) {
            /* Clear service channel states, if we should. */
            if (ctrl::IsDisconnected(state)) {
                this->ClearServiceChannelStates();
            }

            /* Transition our state. */
            m_prev_state = m_state;
            m_state      = state;
        }
    }

    void HtcctrlStateMachine::ClearServiceChannelStates() {
        /* Clear all values in our map. */
        for (auto &pair : m_map) {
            pair.second = {};
        }
    }

    bool HtcctrlStateMachine::IsUnsupportedServiceChannelToShutdown(const impl::ChannelInternalType &channel) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        auto it = m_map.find(channel);
        return it != m_map.end() && it->second.connect == ServiceChannelConnect_ConnectingChecked && it->second.support == ServiceChannelSupport_Unsupported;
    }

    bool HtcctrlStateMachine::IsConnectable(const impl::ChannelInternalType &channel) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        auto it = m_map.find(channel);
        return ctrl::IsConnected(m_state) && (it == m_map.end() || it->second.connect != ServiceChannelConnect_ConnectingChecked);
    }

    void HtcctrlStateMachine::SetNotConnecting(const impl::ChannelInternalType &channel) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        auto it = m_map.find(channel);
        if (it != m_map.end() && it->second.connect != ServiceChannelConnect_ConnectingChecked) {
            it->second.connect = ServiceChannelConnect_NotConnecting;
        }
    }

}
