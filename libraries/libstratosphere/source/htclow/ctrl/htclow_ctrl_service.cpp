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
#include "htclow_ctrl_state.hpp"
#include "htclow_ctrl_state_machine.hpp"
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

    Result HtcctrlService::CheckReceivedHeader(const HtcctrlPacketHeader &header) const {
        /* Check the packet signature. */
        AMS_ASSERT(header.signature == HtcctrlSignature);

        /* Validate version. */
        R_UNLESS(header.version == 1, htclow::ResultProtocolError());

        /* Switch on the packet type. */
        switch (header.packet_type) {
            case HtcctrlPacketType_ConnectFromHost:
            case HtcctrlPacketType_SuspendFromHost:
            case HtcctrlPacketType_ResumeFromHost:
            case HtcctrlPacketType_DisconnectFromHost:
            case HtcctrlPacketType_BeaconQuery:
                R_UNLESS(header.body_size == 0, htclow::ResultProtocolError());
                break;
            case HtcctrlPacketType_ReadyFromHost:
                R_UNLESS(0 <= header.body_size && header.body_size <= sizeof(HtcctrlPacketBody), htclow::ResultProtocolError());
                break;
            default:
                return htclow::ResultProtocolError();
        }

        return ResultSuccess();
    }

    Result HtcctrlService::ProcessReceivePacket(const HtcctrlPacketHeader &header, const void *body, size_t body_size) {
        /* TODO */
        AMS_ABORT("HtcctrlService::ProcessReceivePacket");
    }

    Result HtcctrlService::NotifyDriverConnected() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        if (m_state_machine->GetHtcctrlState() == HtcctrlState_Sleep) {
            R_TRY(this->SetState(HtcctrlState_ExitSleep));
        } else {
            R_TRY(this->SetState(HtcctrlState_DriverConnected));
        }

        return ResultSuccess();
    }

    Result HtcctrlService::NotifyDriverDisconnected() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        if (m_state_machine->GetHtcctrlState() == HtcctrlState_EnterSleep) {
            R_TRY(this->SetState(HtcctrlState_Sleep));
        } else {
            R_TRY(this->SetState(HtcctrlState_DriverDisconnected));
        }

        return ResultSuccess();
    }

    Result HtcctrlService::SetState(HtcctrlState state) {
        /* Set the state. */
        bool did_transition;
        R_TRY(m_state_machine->SetHtcctrlState(std::addressof(did_transition), state));

        /* Reflect the state transition, if one occurred. */
        if (did_transition) {
            this->ReflectState();
        }

        return ResultSuccess();
    }

    void HtcctrlService::ReflectState() {
        /* If our connected status changed, update. */
        if (m_state_machine->IsConnectedStatusChanged()) {
            m_mux->UpdateChannelState();
        }

        /* If our sleeping status changed, update. */
        if (m_state_machine->IsSleepingStatusChanged()) {
            m_mux->UpdateMuxState();
        }

        /* Broadcast our state transition. */
        m_condvar.Broadcast();
    }

}
