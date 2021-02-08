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
#include "htclow_ctrl_settings_holder.hpp"
#include "htclow_ctrl_send_buffer.hpp"

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
            u8 m_1100[0x1000];
            HtcctrlPacketFactory *m_packet_factory;
            HtcctrlStateMachine *m_state_machine;
            mux::Mux *m_mux;
            os::Event m_event;
            HtcctrlSendBuffer m_send_buffer;
            os::SdkMutex m_mutex;
            os::SdkConditionVariable m_condvar;
            u8 m_2170[0x1000];
            s16 m_version;
        private:
            const char *GetConnectionType(impl::DriverType driver_type) const;

            void UpdateBeaconResponse(const char *connection);
        public:
            HtcctrlService(HtcctrlPacketFactory *pf, HtcctrlStateMachine *sm, mux::Mux *mux);
    };

}
