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
#include "htclow_mux_channel_impl.hpp"
#include "../ctrl/htclow_ctrl_state_machine.hpp"

namespace ams::htclow::mux {

    void ChannelImpl::SetVersion(s16 version) {
        /* Sanity check the version. */
        AMS_ASSERT(version <= ProtocolVersion);

        /* Set version. */
        m_version = version;
        m_send_buffer.SetVersion(version);
    }

    void ChannelImpl::UpdateState() {
        /* Check if shutdown must be forced. */
        if (m_state_machine->IsUnsupportedServiceChannelToShutdown(m_channel)) {
            this->ShutdownForce();
        }

        /* Check if we're readied. */
        if (m_state_machine->IsReadied()) {
            m_task_manager->NotifyConnectReady();
        }

        /* Update our state transition. */
        if (m_state_machine->IsConnectable(m_channel)) {
            if (m_state == ChannelState_Unconnectable) {
                this->SetState(ChannelState_Connectable);
            }
        } else if (m_state_machine->IsUnconnectable()) {
            if (m_state == ChannelState_Connectable) {
                this->SetState(ChannelState_Unconnectable);
                m_state_machine->SetNotConnecting(m_channel);
            } else if (m_state == ChannelState_Connected) {
                this->ShutdownForce();
            }
        }
    }

    void ChannelImpl::ShutdownForce() {
        /* Clear our send buffer. */
        m_send_buffer.Clear();

        /* Set our state to shutdown. */
        this->SetState(ChannelState_Shutdown);
    }

    void ChannelImpl::SetState(ChannelState state) {
        /* Check that we can perform the transition. */
        AMS_ABORT_UNLESS(IsStateTransitionAllowed(m_state, state));

        /* Perform the transition. */
        this->SetStateWithoutCheck(state);
    }

    void ChannelImpl::SetStateWithoutCheck(ChannelState state) {
        /* Change our state. */
        if (m_state != state) {
            m_state = state;
            m_state_change_event.Signal();
        }

        /* If relevant, notify disconnect. */
        if (m_state == ChannelState_Shutdown) {
            m_task_manager->NotifyDisconnect(m_channel);
        }
    }

}
