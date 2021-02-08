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
#include "htclow_ctrl_service.hpp"
#include "../mux/htclow_mux.hpp"

namespace ams::htclow::ctrl {

    namespace {

        constexpr const char BeaconPacketResponseTemplate[] =
            "{\r\n"
            "  \"Spec\" : \"%s\",\r\n"
            "  \"Conn\" : \"%s\",\r\n"
            "  \"HW\" : \"%s\",\r\n"
            "  \"Name\" : \"%s\",\r\n"
            "  \"SN\" : \"%s\",\r\n"
            "  \"FW\" : \"%s\",\r\n"
            "  \"Prot\" : \"%d\"\r\n"
            "}\r\n";

    }

    HtcctrlService::HtcctrlService(HtcctrlPacketFactory *pf, HtcctrlStateMachine *sm, mux::Mux *mux)
        : m_settings_holder(), m_beacon_response(), m_1100(), m_packet_factory(pf), m_state_machine(sm), m_mux(mux), m_event(os::EventClearMode_ManualClear),
          m_send_buffer(pf), m_mutex(), m_condvar(), m_2170(), m_version(ProtocolVersion)
    {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Set the mux version. */
        m_mux->SetVersion(m_version);

        /* Update our beacon response. */
        this->UpdateBeaconResponse(this->GetConnectionType(impl::DriverType::Unknown));
    }

    const char *HtcctrlService::GetConnectionType(impl::DriverType driver_type) const {
        switch (driver_type) {
            case impl::DriverType::Socket:       return "TCP";
            case impl::DriverType::Usb:          return "USB-gen2";
            case impl::DriverType::PlainChannel: return "HBPC-gen2";
            default:                             return "Unknown";
        }
    }

    void HtcctrlService::UpdateBeaconResponse(const char *connection) {
        /* Load settings into the holder. */
        m_settings_holder.LoadSettings();

        /* Print our beacon response. */
        util::SNPrintf(m_beacon_response, sizeof(m_beacon_response), BeaconPacketResponseTemplate,
            m_settings_holder.GetSpec(),
            connection,
            m_settings_holder.GetHardwareType(),
            m_settings_holder.GetTargetName(),
            m_settings_holder.GetSerialNumber(),
            m_settings_holder.GetFirmwareVersion(),
            ProtocolVersion
        );
    }

    void HtcctrlService::SetDriverType(impl::DriverType driver_type) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Update our beacon response. */
        this->UpdateBeaconResponse(this->GetConnectionType(driver_type));
    }

}
