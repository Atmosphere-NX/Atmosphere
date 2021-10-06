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
#include <stratosphere.hpp>
#include "htclow_ctrl_service.hpp"
#include "htclow_ctrl_state.hpp"
#include "htclow_ctrl_state_machine.hpp"
#include "htclow_ctrl_packet_factory.hpp"
#include "htclow_service_channel_parser.hpp"
#include "htclow_ctrl_service_channels.hpp"
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
        : m_settings_holder(), m_beacon_response(), m_information_body(), m_packet_factory(pf), m_state_machine(sm), m_mux(mux), m_event(os::EventClearMode_ManualClear),
          m_send_buffer(pf), m_mutex(), m_condvar(), m_service_channels_packet(), m_version(ProtocolVersion)
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

    void HtcctrlService::UpdateInformationBody(const char *status) {
        util::SNPrintf(m_information_body, sizeof(m_information_body), "{\r\n  \"Status\" : \"%s\"\r\n}\r\n", status);
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
                R_UNLESS(header.body_size <= sizeof(HtcctrlPacketBody), htclow::ResultProtocolError());
                break;
            default:
                return htclow::ResultProtocolError();
        }

        return ResultSuccess();
    }

    Result HtcctrlService::ProcessReceivePacket(const HtcctrlPacketHeader &header, const void *body, size_t body_size) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        switch (header.packet_type) {
            case HtcctrlPacketType_ConnectFromHost:
                return this->ProcessReceiveConnectPacket();
            case HtcctrlPacketType_ReadyFromHost:
                return this->ProcessReceiveReadyPacket(body, body_size);
            case HtcctrlPacketType_SuspendFromHost:
                return this->ProcessReceiveSuspendPacket();
            case HtcctrlPacketType_ResumeFromHost:
                return this->ProcessReceiveResumePacket();
            case HtcctrlPacketType_DisconnectFromHost:
                return this->ProcessReceiveDisconnectPacket();
            case HtcctrlPacketType_BeaconQuery:
                return this->ProcessReceiveBeaconQueryPacket();
            default:
                return this->ProcessReceiveUnexpectedPacket();
        }
    }

    Result HtcctrlService::ProcessReceiveConnectPacket() {
        /* Try to transition to sent connect state. */
        if (R_FAILED(this->SetState(HtcctrlState_SentConnectFromHost))) {
            /* We couldn't transition to sent connect. */
            return this->ProcessReceiveUnexpectedPacket();
        }

        /* Send a connect packet. */
        m_send_buffer.AddPacket(m_packet_factory->MakeConnectPacket(m_beacon_response, util::Strnlen(m_beacon_response, sizeof(m_beacon_response)) + 1));

        /* Signal our event. */
        m_event.Signal();

        return ResultSuccess();
    }

    Result HtcctrlService::ProcessReceiveReadyPacket(const void *body, size_t body_size) {
        /* Update our service channels. */
        this->UpdateServiceChannels(body, body_size);

        /* Check that our version is correct. */
        if (m_version < ProtocolVersion) {
            return this->ProcessReceiveUnexpectedPacket();
        }

        /* Set our version. */
        m_version = ProtocolVersion;
        m_mux->SetVersion(m_version);

        /* Set our state. */
        if (R_FAILED(this->SetState(HtcctrlState_SentReadyFromHost))) {
            return this->ProcessReceiveUnexpectedPacket();
        }

        /* Ready ourselves. */
        this->TryReadyInternal();
        return ResultSuccess();
    }

    Result HtcctrlService::ProcessReceiveSuspendPacket() {
        /* Try to set our state to enter sleep. */
        if (R_FAILED(this->SetState(HtcctrlState_EnterSleep))) {
            /* We couldn't transition to sleep. */
            return this->ProcessReceiveUnexpectedPacket();
        }

        return ResultSuccess();
    }

    Result HtcctrlService::ProcessReceiveResumePacket() {
        /* If our state is sent-resume, change to readied. */
        if (m_state_machine->GetHtcctrlState() != HtcctrlState_SentResumeFromTarget || R_FAILED(this->SetState(HtcctrlState_Ready))) {
            /* We couldn't perform a valid resume transition. */
            return this->ProcessReceiveUnexpectedPacket();
        }

        return ResultSuccess();
    }

    Result HtcctrlService::ProcessReceiveDisconnectPacket() {
        /* Set our state. */
        R_TRY(this->SetState(HtcctrlState_Disconnected));

        return ResultSuccess();
    }

    Result HtcctrlService::ProcessReceiveBeaconQueryPacket() {
        /* Send a beacon response packet. */
        m_send_buffer.AddPacket(m_packet_factory->MakeBeaconResponsePacket(m_beacon_response, util::Strnlen(m_beacon_response, sizeof(m_beacon_response)) + 1));

        /* Signal our event. */
        m_event.Signal();

        return ResultSuccess();
    }

    Result HtcctrlService::ProcessReceiveUnexpectedPacket() {
        /* Set our state. */
        R_TRY(this->SetState(HtcctrlState_Error));

        /* Send a disconnection packet. */
        m_send_buffer.AddPacket(m_packet_factory->MakeDisconnectPacket());

        /* Signal our event. */
        m_event.Signal();

        /* Return unexpected packet error. */
        return htclow::ResultHtcctrlReceiveUnexpectedPacket();
    }

    void HtcctrlService::ProcessSendConnectPacket() {
        /* Set our state. */
        const Result result = this->SetState(HtcctrlState_Connected);
        R_ASSERT(result);
    }

    void HtcctrlService::ProcessSendReadyPacket() {
        /* Set our state. */
        if (m_state_machine->GetHtcctrlState() == HtcctrlState_SentReadyFromHost) {
            const Result result = this->SetState(HtcctrlState_Ready);
            R_ASSERT(result);
        }

        /* Update channel states. */
        m_mux->UpdateChannelState();
    }

    void HtcctrlService::ProcessSendSuspendPacket() {
        /* Set our state. */
        const Result result = this->SetState(HtcctrlState_SentSuspendFromTarget);
        R_ASSERT(result);
    }

    void HtcctrlService::ProcessSendResumePacket() {
        /* Set our state. */
        const Result result = this->SetState(HtcctrlState_SentResumeFromTarget);
        R_ASSERT(result);
    }

    void HtcctrlService::ProcessSendDisconnectPacket() {
        /* Set our state. */
        const Result result = this->SetState(HtcctrlState_Disconnected);
        R_ASSERT(result);
    }

    void HtcctrlService::UpdateServiceChannels(const void *body, size_t body_size) {
        /* Copy the packet body to our member. */
        std::memcpy(m_service_channels_packet, body, body_size);

        /* Parse service channels. */
        impl::ChannelInternalType channels[10];
        int num_channels;
        s16 version = m_version;
        ctrl::ParseServiceChannel(std::addressof(version), channels, std::addressof(num_channels), util::size(channels), m_service_channels_packet, body_size);

        /* Update version. */
        m_version = version;

        /* Notify state machine of supported channels. */
        m_state_machine->NotifySupportedServiceChannels(channels, num_channels);
    }

    void HtcctrlService::TryReadyInternal() {
        /* If we can send ready, do so. */
        if (m_state_machine->IsPossibleToSendReady()) {
            /* Print the channels. */
            char channel_str[0x100];
            this->PrintServiceChannels(channel_str, sizeof(channel_str));

            /* Send a ready packet. */
            m_send_buffer.AddPacket(m_packet_factory->MakeReadyPacket(channel_str, util::Strnlen(channel_str, sizeof(channel_str)) + 1));

            /* Signal our event. */
            m_event.Signal();

            /* Set connecting checked in state machine. */
            m_state_machine->SetConnectingChecked();
        }
    }

    bool HtcctrlService::QuerySendPacket(HtcctrlPacketHeader *header, HtcctrlPacketBody *body, int *out_body_size) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        return m_send_buffer.QueryNextPacket(header, body, out_body_size);
    }

    void HtcctrlService::RemovePacket(const HtcctrlPacketHeader &header) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Remove the packet from our buffer. */
        m_send_buffer.RemovePacket(header);

        /* Switch on the packet type. */
        switch (header.packet_type) {
            case HtcctrlPacketType_ConnectFromTarget:
                this->ProcessSendConnectPacket();
                break;
            case HtcctrlPacketType_ReadyFromTarget:
                this->ProcessSendReadyPacket();
                break;
            case HtcctrlPacketType_SuspendFromTarget:
                this->ProcessSendSuspendPacket();
                break;
            case HtcctrlPacketType_ResumeFromTarget:
                this->ProcessSendResumePacket();
                break;
            case HtcctrlPacketType_DisconnectFromTarget:
                this->ProcessSendDisconnectPacket();
                break;
            case HtcctrlPacketType_BeaconResponse:
            case HtcctrlPacketType_InformationFromTarget:
                break;
            default:
                AMS_ABORT("Send unsupported packet 0x%04x\n", static_cast<u32>(header.packet_type));
        }
    }

    void HtcctrlService::TryReady() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        this->TryReadyInternal();
    }

    void HtcctrlService::Disconnect() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        this->DisconnectInternal();
    }

    void HtcctrlService::DisconnectInternal() {
        /* Disconnect, if we need to. */
        if (m_state_machine->IsDisconnectionNeeded()) {
            /* Send a disconnect packet. */
            m_send_buffer.AddPacket(m_packet_factory->MakeDisconnectPacket());

            /* Signal our event. */
            m_event.Signal();

            /* Wait for us to be disconnected. */
            while (!m_state_machine->IsDisconnected()) {
                m_condvar.Wait(m_mutex);
            }
        }
    }

    void HtcctrlService::Resume() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Send resume packet, if we can. */
        if (const auto state = m_state_machine->GetHtcctrlState(); state == HtcctrlState_Sleep || state == HtcctrlState_ExitSleep) {
            /* Send a resume packet. */
            m_send_buffer.AddPacket(m_packet_factory->MakeResumePacket());

            /* Signal our event. */
            m_event.Signal();
        }
    }

    void HtcctrlService::Suspend() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* If we can, perform a suspend. */
        if (m_state_machine->GetHtcctrlState() == HtcctrlState_Ready) {
            /* Send a suspend packet. */
            m_send_buffer.AddPacket(m_packet_factory->MakeSuspendPacket());

            /* Signal our event. */
            m_event.Signal();

            /* Wait for our state to transition. */
            for (auto state = m_state_machine->GetHtcctrlState(); state == HtcctrlState_Ready || state == HtcctrlState_SentSuspendFromTarget; state = m_state_machine->GetHtcctrlState()) {
                m_condvar.Wait(m_mutex);
            }
        } else {
            /* Otherwise, just disconnect. */
            this->DisconnectInternal();
        }
    }

    void HtcctrlService::NotifyAwake() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Update our information. */
        this->UpdateInformationBody("Awake");

        /* Send information to host. */
        this->SendInformation();
    }

    void HtcctrlService::NotifyAsleep() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Update our information. */
        this->UpdateInformationBody("Asleep");

        /* Send information to host. */
        this->SendInformation();
    }

    void HtcctrlService::SendInformation() {
        /* If we need information, send information. */
        if (m_state_machine->IsInformationNeeded()) {
            /* Send an information packet. */
            m_send_buffer.AddPacket(m_packet_factory->MakeInformationPacket(m_information_body, util::Strnlen(m_information_body, sizeof(m_information_body)) + 1));

            /* Signal our event. */
            m_event.Signal();
        }
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

    void HtcctrlService::PrintServiceChannels(char *dst, size_t dst_size) {
        size_t ofs = 0;
        ofs += util::SNPrintf(dst + ofs, dst_size - ofs, "{\r\n  \"Chan\" : [\r\n \"%d:%d:%d\"", static_cast<int>(ServiceChannels[0].module_id), ServiceChannels[0].reserved, static_cast<int>(ServiceChannels[0].channel_id));
        for (size_t i = 1; i < util::size(ServiceChannels); ++i) {
            ofs += util::SNPrintf(dst + ofs, dst_size - ofs, ",\r\n \"%d:%d:%d\"", static_cast<int>(ServiceChannels[i].module_id), ServiceChannels[i].reserved, static_cast<int>(ServiceChannels[i].channel_id));
        }
        ofs += util::SNPrintf(dst + ofs, dst_size - ofs, "\r\n],\r\n  \"Prot\" : %d\r\n}\r\n", ProtocolVersion);
    }

}
