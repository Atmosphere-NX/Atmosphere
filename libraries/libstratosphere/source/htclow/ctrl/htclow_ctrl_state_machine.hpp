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
#include "htclow_ctrl_state.hpp"

namespace ams::htclow::ctrl {

    class HtcctrlStateMachine {
        private:
            enum ServiceChannelSupport {
                ServiceChannelSupport_Unknown     = 0,
                ServiceChannelSupport_Suppported  = 1,
                ServiceChannelSupport_Unsupported = 2,
            };

            enum ServiceChannelConnect {
                ServiceChannelConnect_NotConnecting     = 0,
                ServiceChannelConnect_Connecting        = 1,
                ServiceChannelConnect_ConnectingChecked = 2,
            };

            struct ServiceChannelState {
                ServiceChannelSupport support;
                ServiceChannelConnect connect;
            };

            static constexpr int MaxChannelCount = 10;

            using MapType = util::FixedMap<impl::ChannelInternalType, ServiceChannelState>;

            static constexpr size_t MapRequiredMemorySize = MapType::GetRequiredMemorySize(MaxChannelCount);
        private:
            u8 m_map_buffer[MapRequiredMemorySize];
            MapType m_map;
            HtcctrlState m_state;
            HtcctrlState m_prev_state;
            os::SdkMutex m_mutex;
        public:
            HtcctrlStateMachine();

            HtcctrlState GetHtcctrlState();
            Result SetHtcctrlState(bool *out_transitioned, HtcctrlState state);

            bool IsConnectedStatusChanged();
            bool IsSleepingStatusChanged();

            bool IsInformationNeeded();
            bool IsDisconnectionNeeded();

            bool IsConnected();
            bool IsReadied();
            bool IsUnconnectable();
            bool IsDisconnected();
            bool IsSleeping();

            bool IsPossibleToSendReady();
            bool IsUnsupportedServiceChannelToShutdown(const impl::ChannelInternalType &channel);
            bool IsConnectable(const impl::ChannelInternalType &channel);

            void SetConnecting(const impl::ChannelInternalType &channel);
            void SetNotConnecting(const impl::ChannelInternalType &channel);
            void SetConnectingChecked();

            void NotifySupportedServiceChannels(const impl::ChannelInternalType *channels, int num_channels);
        private:
            void SetStateWithoutCheckInternal(HtcctrlState state);

            bool AreServiceChannelsConnecting();

            void ClearServiceChannelStates();
    };

}
