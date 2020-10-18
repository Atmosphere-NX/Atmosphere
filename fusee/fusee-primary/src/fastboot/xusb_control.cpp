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

#include "xusb_control.h"

#include "usb_types.h"
#include "xusb.h"
#include "xusb_endpoint.h"
#include "../reg/reg_xusb_dev.h"

extern "C" {
#include "../timers.h"
#include "../lib/log.h"
}

#include<string.h>

#include<vapours/results/xusb_gadget_results.hpp>

namespace ams {

    namespace xusb {

        namespace control {
        
            DeviceState device_state = DeviceState::Powered;

            namespace {

                enum class ControlEpState {
                    Idle,
                    DataInStage,
                    DataOutStage,
                    StatusStage,
                } control_ep_state = ControlEpState::Idle;
        
                TRB *control_ep_last_trb = nullptr;
                uint16_t control_ep_seq_num = 0;
            
                void ResetControlEndpoint() {
                    xusb::endpoints[0].Pause();
                    xusb::endpoints[0].ResetAndReloadTransferRing();
                    xusb::endpoints[0].ClearPause();
                
                    control_ep_state = ControlEpState::Idle;
                    control_ep_last_trb = nullptr;
                }
        
                void ChangeDeviceState(DeviceState new_state) {
                    // if we are transitioning in or out of configured state,
                    if ((device_state == DeviceState::Configured) != (new_state == DeviceState::Configured)) {
                        t210::T_XUSB_DEV_XHCI.CTRL().RUN.Set(new_state == DeviceState::Configured).Commit();

                        if (new_state == DeviceState::Configured) {
                            t210::T_XUSB_DEV_XHCI.ST().RC.Clear().Commit();
                            xusb::GetCurrentGadget()->PostConfigure();
                        }
                    }
            
                    switch(new_state) {
                    case DeviceState::Default:
                        device_state = DeviceState::Default;

                        print(SCREEN_LOG_LEVEL_DEBUG, "entering default state, resetting control endpoint state...\n");
                        ResetControlEndpoint();
                        break;
                    case DeviceState::Address:
                        device_state = DeviceState::Address;
                        break;
                    case DeviceState::Configured:
                        device_state = DeviceState::Configured;
                        break;
                    default:
                        print(SCREEN_LOG_LEVEL_ERROR, "ChangeDeviceState(%d) state unknown\n", (int) new_state);
                        break;
                    }
                }

            } // anonymous namespace
                
            Result SendData(void *data, size_t size) {
                TRBBorrow trb;
                    
                R_TRY(xusb::endpoints[0].EnqueueTRB(&trb));
                    
                trb->transfer.InitializeDataStage(true, (void*) data, size);
                trb->transfer.interrupt_on_completion = true;
                trb->transfer.interrupt_on_short_packet = true;
                trb.Release();

                udelay(1000);
                
                xusb::endpoints[0].RingDoorbell(control_ep_seq_num);
                
                control_ep_state = ControlEpState::DataInStage;
                control_ep_last_trb = &*trb;

                return ResultSuccess();
            }
            
            Result SendStatus(bool direction) {
                TRBBorrow trb;
                    
                R_TRY(xusb::endpoints[0].EnqueueTRB(&trb));
                    
                trb->transfer.InitializeStatusStage(direction);
                trb->transfer.interrupt_on_completion = true;
                trb->transfer.interrupt_on_short_packet = true;
                trb.Release();

                udelay(1000);
                
                xusb::endpoints[0].RingDoorbell(control_ep_seq_num);

                control_ep_state = ControlEpState::StatusStage;
                control_ep_last_trb = &*trb;

                return ResultSuccess();
            }

            namespace impl {

                Result SetAddress(uint16_t addr) {
                    print(SCREEN_LOG_LEVEL_DEBUG, "got SET_ADDRESS request(%d)\n", addr);

                    R_UNLESS(addr <= 127, ams::xusb::ResultInvalidAddress());

                    t210::T_XUSB_DEV_XHCI
                        .CTRL()
                        .DEVADR.Set(addr)
                        .Commit();

                    xusb::endpoints.SetEP0DeviceAddress(addr);

                    ChangeDeviceState(DeviceState::Address);
                
                    return SendStatus(true);
                }
            
                Result GetDescriptor(usb::DescriptorType type, uint8_t index, uint16_t language_id, uint16_t length) {
                    print(SCREEN_LOG_LEVEL_DEBUG, "got GET_DESCRIPTOR request(%d, %d, %d, %d)\n", type, index, language_id, length);

                    uint16_t actual_length = 0;
                    const usb::CommonDescriptor *descriptor = nullptr;
                
                    switch(type) {
                    case usb::DescriptorType::DEVICE:
                        R_UNLESS(language_id == 0, xusb::ResultMalformedSetupRequest());
                        R_TRY(xusb::GetCurrentGadget()->GetDeviceDescriptor(index, (const usb::DeviceDescriptor**) &descriptor, &actual_length));
                        break;
                    case usb::DescriptorType::CONFIGURATION:
                        R_UNLESS(language_id == 0, xusb::ResultMalformedSetupRequest());
                        R_TRY(xusb::GetCurrentGadget()->GetConfigurationDescriptor(index, (const usb::ConfigurationDescriptor**) &descriptor, &actual_length));
                        break;
                    case usb::DescriptorType::BOS:
                        R_UNLESS(index == 0, xusb::ResultInvalidDescriptorIndex());
                        R_UNLESS(language_id == 0, xusb::ResultMalformedSetupRequest());
                        R_TRY(xusb::GetCurrentGadget()->GetBOSDescriptor((const usb::BOSDescriptor**) &descriptor, &actual_length));
                        break;
                    case usb::DescriptorType::STRING:
                        R_TRY(xusb::GetCurrentGadget()->GetStringDescriptor(index, language_id, &descriptor, &actual_length));
                        break;
                    case usb::DescriptorType::DEVICE_QUALIFIER:
                        R_UNLESS(language_id == 0, xusb::ResultMalformedSetupRequest());
                        R_TRY(xusb::GetCurrentGadget()->GetDeviceQualifierDescriptor(index, (const usb::DeviceQualifierDescriptor**) &descriptor, &actual_length));
                        break;
                    default:
                        return xusb::ResultUnknownDescriptorType();
                    }
                
                    if (actual_length > length) {
                        actual_length = length;
                    }

                    return SendData((void*) descriptor, actual_length);
                }

                Result SetConfiguration(uint16_t config) {
                    print(SCREEN_LOG_LEVEL_DEBUG, "got SET_CONFIGURATION request(%d)\n", config);

                    R_UNLESS(device_state == DeviceState::Address || device_state == DeviceState::Configured, xusb::ResultInvalidDeviceState());

                    // if we are already configured, deconfigure first.
                    if (device_state == DeviceState::Configured) {
                        R_TRY(xusb::GetCurrentGadget()->Deconfigure());
                        ChangeDeviceState(DeviceState::Address);
                    }

                    // if we need to reconfigure, do it.
                    if (config != 0) {
                        R_TRY(xusb::GetCurrentGadget()->SetConfiguration(config));
                        ChangeDeviceState(DeviceState::Configured);
                    }
                
                    return SendStatus(true);
                }

                Result ClearEndpointHaltFeature(uint8_t index) {
                    xusb::endpoints[EpAddr::Decode(index)].ClearHalt();
                    
                    return SendStatus(true);
                }
                
            } // namespace impl

            Result ProcessEP0TransferEventImpl(TransferEventTRB *event, TRBBorrow transfer) {
                if (event->completion_code == 1 || event->completion_code == 13) {
                    /* Success or short packet. */
                } else if (event->completion_code == 223) { // sequence number error
                    /* Sequence number error; skip to latest packet. */
                    control_ep_state = ControlEpState::Idle;
                } else {
                    /* Error. */
                    control_ep_state = ControlEpState::Idle;
                }
            
                if (&*transfer != control_ep_last_trb) {
                    /* This can happen when we skip packets. */
                    print(SCREEN_LOG_LEVEL_ERROR, "got transfer event for wrong TRB (got %p, expected %p)\n", &*transfer, control_ep_last_trb);
                    return xusb::ResultUnexpectedTRB();
                } else {
                    control_ep_last_trb = nullptr;
                
                    switch(control_ep_state) {
                    case ControlEpState::Idle:
                        return xusb::ResultInvalidControlEndpointState();
                    case ControlEpState::DataInStage:
                        return SendStatus(false); // OUT
                    case ControlEpState::DataOutStage:
                        return SendStatus(false); // IN
                    case ControlEpState::StatusStage:
                        control_ep_state = ControlEpState::Idle;
                        return ResultSuccess();
                    default:
                        return ResultSuccess();
                    }
                }
            }
            
            void ProcessEP0TransferEvent(TransferEventTRB *event, TRBBorrow transfer) {
                Result r = ProcessEP0TransferEventImpl(event, std::move(transfer));
                
                if (r.IsFailure()) {
                    print(SCREEN_LOG_LEVEL_DEBUG, "failed to handle ep0 transfer event: 0x%x\n", r.GetValue());
                    xusb::endpoints[0].Halt();
                }
            }

            Result ProcessSetupPacketEventImpl(SetupPacketEventTRB *trb) {
                R_UNLESS(trb->seq_num != 0xfffe, xusb::ResultInvalidSetupPacketSequenceNumber());
                R_UNLESS(trb->seq_num != 0xffff, xusb::ResultInvalidSetupPacketSequenceNumber());

                R_UNLESS(device_state >= DeviceState::Default && device_state <= DeviceState::Configured, xusb::ResultInvalidDeviceState());

                R_UNLESS(control_ep_state == ControlEpState::Idle, xusb::ResultControlEndpointBusy());

                control_ep_seq_num = trb->seq_num;
                xusb::endpoints[0].ClearHalt();

                /* Let gadget try to service request first. */

                do {
                    R_TRY_CATCH(xusb::GetCurrentGadget()->HandleSetupRequest(trb->packet)) {
                        R_CATCH(xusb::ResultUnknownSetupRequest) { break; }
                    } R_END_TRY_CATCH;

                    return ResultSuccess();
                } while (0);

                /* If gadget did not recognize the request, handle it ourselves. */
                
                switch((usb::SetupPacket::StandardRequest) trb->packet.bRequest) {
                case usb::SetupPacket::StandardRequest::CLEAR_FEATURE:
                    if (trb->packet.bmRequestType == usb::SetupPacket::PackRequestType(
                           usb::SetupPacket::RequestDirection::HostToDevice,
                           usb::SetupPacket::RequestType::Standard,
                           usb::SetupPacket::RequestRecipient::Endpoint) &&
                       trb->packet.wValue == (uint8_t) usb::EndpointFeatureSelector::ENDPOINT_HALT) {
                        R_UNLESS(trb->packet.wLength == 0, xusb::ResultMalformedSetupRequest());
                        
                        return impl::ClearEndpointHaltFeature(trb->packet.wIndex);
                    } else {
                        return xusb::ResultUnknownSetupRequest();
                    }
                case usb::SetupPacket::StandardRequest::SET_ADDRESS:
                    if (trb->packet.bmRequestType != usb::SetupPacket::PackRequestType(
                           usb::SetupPacket::RequestDirection::HostToDevice,
                           usb::SetupPacket::RequestType::Standard,
                           usb::SetupPacket::RequestRecipient::Device) ||
                       trb->packet.wIndex != 0 ||
                       trb->packet.wLength != 0) {
                        return xusb::ResultMalformedSetupRequest();
                    }

                    if (device_state == DeviceState::Configured) {
                        return xusb::ResultInvalidDeviceState();
                    }
                
                    return impl::SetAddress(trb->packet.wValue);
                    
                case usb::SetupPacket::StandardRequest::GET_DESCRIPTOR:
                    if (trb->packet.bmRequestType != usb::SetupPacket::PackRequestType(
                           usb::SetupPacket::RequestDirection::DeviceToHost,
                           usb::SetupPacket::RequestType::Standard,
                           usb::SetupPacket::RequestRecipient::Device)) {
                        return xusb::ResultMalformedSetupRequest();
                    }

                    return impl::GetDescriptor(
                        (usb::DescriptorType) (trb->packet.wValue >> 8),
                        trb->packet.wValue & 0xff,
                        trb->packet.wIndex,
                        trb->packet.wLength);
                    
                case usb::SetupPacket::StandardRequest::SET_CONFIGURATION:
                    if (trb->packet.bmRequestType != usb::SetupPacket::PackRequestType(
                           usb::SetupPacket::RequestDirection::HostToDevice,
                           usb::SetupPacket::RequestType::Standard,
                           usb::SetupPacket::RequestRecipient::Device) ||
                       trb->packet.wIndex != 0 ||
                       trb->packet.wLength != 0) {
                        return xusb::ResultMalformedSetupRequest();
                    }

                    return impl::SetConfiguration(trb->packet.wValue);
                    
                default:
                    return xusb::ResultUnknownSetupRequest();
                }
            }
            
            void ProcessSetupPacketEvent(SetupPacketEventTRB *trb) {
                Result r = ProcessSetupPacketEventImpl(trb);

                if (r.IsFailure()) {
                    print(SCREEN_LOG_LEVEL_WARNING, "error 0x%x while handling setup request\n", r.GetValue());
                    print(SCREEN_LOG_LEVEL_WARNING, "  bmRequestType: %d\n", trb->packet.bmRequestType);
                    print(SCREEN_LOG_LEVEL_WARNING, "  bRequest: %d\n", trb->packet.bRequest);
                    print(SCREEN_LOG_LEVEL_WARNING, "  wValue: %d\n", trb->packet.wValue);
                    print(SCREEN_LOG_LEVEL_WARNING, "  wIndex: %d\n", trb->packet.wIndex);
                    print(SCREEN_LOG_LEVEL_WARNING, "  wLenth: %d\n", trb->packet.wLength);
                    xusb::endpoints[0].Halt();
                }
            }
    
            void ProcessPortStatusChangeEvent(AbstractTRB *trb) {
                auto portsc = t210::T_XUSB_DEV_XHCI.PORTSC();
                auto porthalt = t210::T_XUSB_DEV_XHCI.PORTHALT();

                print(SCREEN_LOG_LEVEL_DEBUG, "port status changed (0x%08x)\n", portsc.read_value);
        
                if (portsc.CSC) { // connect status change
                    int port_speed = portsc.PS.Get();
                    print(SCREEN_LOG_LEVEL_DEBUG, "connect status changed (port speed %d)\n", port_speed);
                    portsc.CSC.Clear().Commit();
                }

                if (portsc.PRC.Get()) { // port reset change
                    print(SCREEN_LOG_LEVEL_DEBUG, "port reset changed (%d)\n", portsc.PR.Get());

                    if (!portsc.PR.Get()) {
                        ChangeDeviceState(DeviceState::Default);
                    }

                    portsc.PRC.Clear().Commit();
                }

                if (portsc.WRC) { // warm port reset change
                    t210::T_XUSB_DEV_XHCI.PORTHALT().HALT_LTSSM.Set(0).Commit();

                    print(SCREEN_LOG_LEVEL_DEBUG, "warm port reset changed (%d)\n", portsc.WPR.Get());

                    portsc.WRC.Clear().Commit();
                }

                if (porthalt.STCHG_REQ) {
                    print(SCREEN_LOG_LEVEL_DEBUG, "porthalt STCHG_REQ\n");
                    porthalt.HALT_LTSSM.Set(0).Commit();
                }

                if (portsc.PLC) { // port link state change
                    print(SCREEN_LOG_LEVEL_DEBUG, "port link state changed (%d)\n", portsc.PLS.Get());

                    if (portsc.PLS.Get() == 3) { // U3
                        ChangeDeviceState(DeviceState::Default);
                    }
                
                    portsc.PLC.Clear().Commit();
                }

                if (portsc.CEC) { // port configuration link error
                    print(SCREEN_LOG_LEVEL_DEBUG, "port configuration link error\n");
                    portsc.CEC.Clear().Commit();
                }
            }
    
        } // namespace control

    } // namespace xusb

} // namespace ams
