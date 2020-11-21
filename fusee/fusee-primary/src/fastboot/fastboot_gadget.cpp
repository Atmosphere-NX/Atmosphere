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

#include "fastboot_gadget.h"

#include "fastboot.h"

#include "xusb.h"
#include "xusb_control.h"
#include "xusb_endpoint.h"

extern "C" {
#include "../lib/log.h"
#include "../lib/vsprintf.h"
#include "../utils.h"
#include "../btn.h"
}

#include "fastboot_descriptors.inc"

#include<vapours/util/util_scope_guard.hpp>
#include<algorithm>
#include<cstdarg>

namespace ams {

    namespace fastboot {

        namespace {
            xusb::TRB in_ring[5];
            xusb::TRB out_ring[5];
        
            namespace ep {
                static constexpr auto out = xusb::endpoints[xusb::EpAddr {1, xusb::Dir::Out}];
                static constexpr auto in = xusb::endpoints[xusb::EpAddr {1, xusb::Dir::In}];
            }
        
        } // anonymous namespace

        FastbootGadget::FastbootGadget(uint8_t *download_buffer, size_t download_buffer_size) :
            impl(*this),
            download_buffer(download_buffer),
            download_buffer_size(download_buffer_size) {
        }

        fastboot_return FastbootGadget::Run() {
            bool usb_error = false;
            uint32_t last_button = btn_read();
            while (this->state < State::Exit || usb_error) {
                if (!usb_error) {
                    xusb::Process();
                }

                if (this->state == State::UsbError && !usb_error) {
                    print((ScreenLogLevel) (SCREEN_LOG_LEVEL_NONE | SCREEN_LOG_LEVEL_NO_PREFIX), "\nFastboot encountered a USB error (0x%x).\nPress one of the buttons listed above to continue.\n", this->usb_error.GetValue());
                    usb_error = true;
                }
            
                uint32_t button = btn_read();
                if (button & BTN_VOL_UP && !(last_button & BTN_VOL_UP)) {
                    /* Reboot to RCM. */
                    pmc_reboot(0);
                    return FASTBOOT_INVALID;
                }

                if (button & BTN_VOL_DOWN && !(last_button & BTN_VOL_DOWN)) {
                    /* Boot normally. */
                    print(SCREEN_LOG_LEVEL_INFO, "Booting normally.\n");
                    return FASTBOOT_LOAD_STAGE2;
                }

                if (button & BTN_POWER && !(last_button & BTN_POWER)) {
                    /* Reboot to fusee-primary. */
                    reboot_to_self();
                    return FASTBOOT_INVALID;
                }
                
                last_button = button;
            }

            switch(this->state) {
            case State::Exit:
                return FASTBOOT_LOAD_STAGE2;
            case State::Reboot:
                reboot_to_self();
                return FASTBOOT_INVALID;
            case State::Chainload:
                return FASTBOOT_CHAINLOAD;
            default:
                return FASTBOOT_INVALID;
            }
        }

        void FastbootGadget::FormatResponse(ResponseToken token, const char *fmt, std::va_list args) {
            const char *token_str = nullptr;
        
            switch(token) {
            case ResponseToken::OKAY:
                token_str = "OKAY";
                break;
            case ResponseToken::INFO:
                token_str = "INFO";
                break;
            case ResponseToken::FAIL:
                token_str = "FAIL";
                break;
            case ResponseToken::DATA:
                token_str = "DATA";
                break;
            }

            strcpy(this->response_buffer, token_str);
            vsnprintf(this->response_buffer + 4, sizeof(this->response_buffer) - 4, fmt, args);
        }
        
        Result FastbootGadget::SendResponse(ResponseDisposition disposition, ResponseToken token, const char *fmt, ...) {
            std::va_list args;
            va_start(args, fmt);
            
            FormatResponse(token, fmt, args);
            
            va_end(args);
        
            R_TRY(ep::in.TransferNormal((void*) this->response_buffer, strlen(this->response_buffer), &last_trb));
            
            this->response_disposition = disposition;
            this->state = State::SendingResponse;

            return ResultSuccess();
        }

        static const size_t max_trb_size = 0x10000; // 64 KiB
    
        Result FastbootGadget::PrepareDownload(size_t size) {
            R_UNLESS(size <= this->download_buffer_size, xusb::ResultDownloadTooLarge());

            this->download_head = 0;
            this->download_size = size;
            this->download_needs_zlp = (size % max_trb_size) == 0;

            return ResultSuccess();
        }

        size_t FastbootGadget::GetMaxDownloadSize() const {
            return this->download_buffer_size;
        }

        size_t FastbootGadget::GetLastDownloadSize() const {
            return this->download_size;
        }

        void *FastbootGadget::GetLastDownloadBuffer() const {
            return this->download_buffer;
        }
    
        Result FastbootGadget::ReadHostCommand() {
            R_TRY(ep::out.TransferNormal((void*) this->command_buffer, 64, &last_trb));
            
            this->state = State::WaitingForHostCommand;

            return ResultSuccess();
        }

        Result FastbootGadget::QueueTRBsForReceive() {        
            xusb::TRBBorrow trb;

            bool has_queued_any = false;
            
            while (ep::out.GetFreeTRBCount() > 0 && (this->download_head < this->download_size || this->download_needs_zlp)) {
                size_t remaining_in_download = this->download_size - this->download_head;
                size_t num_trbs_remaining = (remaining_in_download + max_trb_size - 1) / max_trb_size;

                if (this->download_needs_zlp) {
                    num_trbs_remaining++;
                }
                
                size_t td_size = std::min(std::min(num_trbs_remaining, (size_t) 32), (size_t) 32) - 1;
                
                /* Attempt to queue a TRB. */
                
                size_t attempted_size = std::min(remaining_in_download, max_trb_size);
                
                /* If TRBBorrow is already holding a TRB, it will be queued implicitly when EnqueueTRB overwrites it. */
                R_TRY(ep::out.EnqueueTRB(&trb, num_trbs_remaining > 0));

                R_UNLESS(&*trb != nullptr, xusb::ResultTransferRingFull());
                
                trb->transfer.InitializeNormal(download_buffer + download_head, attempted_size);

                if (td_size > 0) {
                    trb->transfer.chain_bit = 1;
                    trb->transfer.td_size = td_size;
                }
                
                if (attempted_size == 0) {
                    this->download_needs_zlp = false;
                } else {
                    this->download_head+= attempted_size;
                }

                has_queued_any = true;
            }

            if (has_queued_any) {
                trb->transfer.interrupt_on_completion = true;
                trb->transfer.interrupt_on_short_packet = true;
                trb.Release();

                if (this->download_head == this->download_size && !this->download_needs_zlp) {
                    last_trb = &*trb;
                }

                this->state = State::DataPhaseReceive;
            
                ep::out.RingDoorbell();
            }

            return ResultSuccess();
        }

        /* xusb::Gadget implementation */

        Result FastbootGadget::ProcessTransferEventImpl(xusb::TransferEventTRB *event, xusb::TRBBorrow transfer) {
            if (event->completion_code != 1 && event->completion_code != 13) { // success or short packet
                print(SCREEN_LOG_LEVEL_DEBUG, "failing due to transfer completion with bad code %d\n", event->completion_code);

                return xusb::ResultUnexpectedCompletionCode();
            }
            
            switch(this->state) {
            case State::WaitingForHostCommand:
                R_UNLESS(event->ep_id == ep::out.GetIndex(), xusb::ResultUnexpectedEndpoint());
                R_UNLESS(&*transfer == this->last_trb, xusb::ResultUnexpectedTRB());

                {
                    size_t received_size = transfer->transfer.transfer_length - event->trb_transfer_length;
                    this->command_buffer[received_size] = 0;

                    return this->impl.ProcessCommand(this->command_buffer);
                }
                
            case State::SendingResponse:
                R_UNLESS(event->ep_id == ep::in.GetIndex(), xusb::ResultUnexpectedEndpoint());
                R_UNLESS(&*transfer == this->last_trb, xusb::ResultUnexpectedTRB());

                switch(this->response_disposition) {
                case ResponseDisposition::ReadHostCommand:
                    return this->ReadHostCommand();
                case ResponseDisposition::Download:
                    this->last_trb = nullptr;
                    return this->QueueTRBsForReceive();
                case ResponseDisposition::Okay:
                    return this->SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::OKAY, "");
                case ResponseDisposition::Continue:
                    this->state = State::Continuing;
                    return this->impl.Continue();
                case ResponseDisposition::Reboot:
                    this->state = State::Reboot;
                    return ResultSuccess();
                case ResponseDisposition::Chainload:
                    this->state = State::Chainload;
                    return ResultSuccess();
                default:
                    return ResultSuccess();
                }
                
            case State::DataPhaseReceive:
                R_UNLESS(event->ep_id == ep::out.GetIndex(), xusb::ResultUnexpectedEndpoint());

                /* Release completed TRBs back to ring early so we can reuse them. */
                transfer.Release();
            
                if (&*transfer == last_trb) {
                    return this->SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::OKAY, "");
                } else {
                    return this->QueueTRBsForReceive();
                }
                
            case State::DataPhaseTransmit:
                /* TODO, if we ever implement a command that involves sending data. */
                return ResultSuccess();

            case State::Continuing:
                return this->impl.Continue();

            default:
                return ResultSuccess();
            }
        }

        void FastbootGadget::ProcessTransferEvent(xusb::TransferEventTRB *trb, xusb::TRBBorrow transfer) {
            Result r = this->ProcessTransferEventImpl(trb, std::move(transfer));
            
            if (r.IsFailure()) {
                print(SCREEN_LOG_LEVEL_ERROR, "fastboot error while processing transfer event: 0x%x\n", r.GetValue());
                this->state = State::UsbError;
                this->usb_error = r;
            }
        }

        Result FastbootGadget::HandleSetupRequest(usb::SetupPacket &packet) {
            if (packet.bmRequestType == usb::SetupPacket::PackRequestType(
                   usb::SetupPacket::RequestDirection::DeviceToHost,
                   usb::SetupPacket::RequestType::Vendor,
                   usb::SetupPacket::RequestRecipient::Device) &&
               packet.bRequest == descriptors::bMS_VendorCode) {
                /* Retrieve MS OS 2.0 vendor-specific descriptor */
                R_UNLESS(packet.wValue == 0, xusb::ResultMalformedSetupRequest());
                R_UNLESS(packet.wIndex == usb::ms::MS_OS_20_DESCRIPTOR_INDEX, xusb::ResultMalformedSetupRequest());

                uint16_t actual_length = sizeof(descriptors::ms_os20_descriptor);
                if (actual_length > packet.wLength) {
                    actual_length = packet.wLength;
                }

                return xusb::control::SendData((void*) &descriptors::ms_os20_descriptor, actual_length);
            }
            
            return xusb::ResultUnknownSetupRequest();
        }
        
        Result FastbootGadget::GetDeviceDescriptor(uint8_t index, const usb::DeviceDescriptor **descriptor, uint16_t *length) {
            R_UNLESS(index == 0, xusb::ResultInvalidDescriptorIndex());

            *descriptor = &descriptors::device_descriptor;
            *length = sizeof(descriptors::device_descriptor);
        
            return ResultSuccess();
        }
    
        Result FastbootGadget::GetDeviceQualifierDescriptor(uint8_t index, const usb::DeviceQualifierDescriptor **descriptor, uint16_t *length) {
            R_UNLESS(index == 0, xusb::ResultInvalidDescriptorIndex());

            *descriptor = &descriptors::device_qualifier_descriptor;
            *length = sizeof(descriptors::device_qualifier_descriptor);
        
            return ResultSuccess();
        }
    
        Result FastbootGadget::GetConfigurationDescriptor(uint8_t index, const usb::ConfigurationDescriptor **descriptor, uint16_t *length) {
            R_UNLESS(index == 0, xusb::ResultInvalidDescriptorIndex());

            *descriptor = &descriptors::whole_configuration_descriptor.configuration_descriptor;
            *length = sizeof(descriptors::whole_configuration_descriptor);
        
            return ResultSuccess();
        }

        Result FastbootGadget::GetBOSDescriptor(const usb::BOSDescriptor **descriptor, uint16_t *length) {
            *descriptor = &descriptors::bos_container.bos_descriptor;
            *length = sizeof(descriptors::bos_container);
        
            return ResultSuccess();
        }

        Result FastbootGadget::GetStringDescriptor(uint8_t index, uint16_t language, const usb::CommonDescriptor **descriptor, uint16_t *length) {
            R_UNLESS(language == 0x0409 || (language == 0 && index == 0), xusb::ResultInvalidDescriptorIndex());
            R_UNLESS(descriptors::sd_indexer.GetStringDescriptor(index, language, descriptor, length), xusb::ResultInvalidDescriptorIndex());

            return ResultSuccess();
        }
            
        Result FastbootGadget::SetConfiguration(uint16_t configuration) {
            R_UNLESS(configuration <= 1, xusb::ResultInvalidConfiguration());

            xusb::endpoints.ClearNonControlEndpoints();

            ep::out.Initialize(descriptors::whole_configuration_descriptor.endpoint_out_descriptor, out_ring, std::size(out_ring));
            ep::in .Initialize(descriptors::whole_configuration_descriptor.endpoint_in_descriptor,  in_ring,  std::size(in_ring));
                
            return ResultSuccess();
        }

        Result FastbootGadget::Deconfigure() {
            ep::out.Disable();
            ep::in.Disable();

            return ResultSuccess();
        }

        void FastbootGadget::PostConfigure() {
            this->state = State::Invalid;

            ReadHostCommand();
        }
    
    } // namespace fastboot

} // namespace ams
