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
#include "htclow_ctrl_settings_holder.hpp"
#include "htclow_ctrl_send_buffer.hpp"
#include "htclow_ctrl_state.hpp"

namespace ams::htclow {

    namespace ctrl {

        class HtcctrlPacketFactory;
        class HtcctrlStateMachine;

    }

    namespace mux {

        class Mux;

    }

}

namespace ams::htclow::ctrl {

    class HtcctrlService {
        private:
            SettingsHolder m_settings_holder;
            char m_beacon_response[0x1000];
            char m_information_body[0x1000];
            HtcctrlPacketFactory *m_packet_factory;
            HtcctrlStateMachine *m_state_machine;
            mux::Mux *m_mux;
            os::Event m_event;
            HtcctrlSendBuffer m_send_buffer;
            os::SdkMutex m_mutex;
            os::SdkConditionVariable m_condvar;
            char m_service_channels_packet[0x1000];
            s16 m_version;
        private:
            const char *GetConnectionType(impl::DriverType driver_type) const;

            void UpdateBeaconResponse(const char *connection);
            void UpdateInformationBody(const char *status);

            void SendInformation();

            Result ProcessReceiveConnectPacket();
            Result ProcessReceiveReadyPacket(const void *body, size_t body_size);
            Result ProcessReceiveSuspendPacket();
            Result ProcessReceiveResumePacket();
            Result ProcessReceiveDisconnectPacket();
            Result ProcessReceiveBeaconQueryPacket();
            Result ProcessReceiveUnexpectedPacket();

            void ProcessSendConnectPacket();
            void ProcessSendReadyPacket();
            void ProcessSendSuspendPacket();
            void ProcessSendResumePacket();
            void ProcessSendDisconnectPacket();

            void UpdateServiceChannels(const void *body, size_t body_size);

            void PrintServiceChannels(char *dst, size_t dst_size);

            void TryReadyInternal();
            void DisconnectInternal();

            Result SetState(HtcctrlState state);
            void ReflectState();
        public:
            HtcctrlService(HtcctrlPacketFactory *pf, HtcctrlStateMachine *sm, mux::Mux *mux);

            void SetDriverType(impl::DriverType driver_type);

            os::EventType *GetSendPacketEvent() { return m_event.GetBase(); }

            Result CheckReceivedHeader(const HtcctrlPacketHeader &header) const;
            Result ProcessReceivePacket(const HtcctrlPacketHeader &header, const void *body, size_t body_size);

            bool QuerySendPacket(HtcctrlPacketHeader *header, HtcctrlPacketBody *body, int *out_body_size);
            void RemovePacket(const HtcctrlPacketHeader &header);

            void TryReady();
            void Disconnect();

            void Resume();
            void Suspend();

            void NotifyAwake();
            void NotifyAsleep();

            Result NotifyDriverConnected();
            Result NotifyDriverDisconnected();
    };

}
