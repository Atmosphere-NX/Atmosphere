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

#include "xusb.h"
#include "fastboot.h"
#include "fastboot_impl.h"

#include<cstdarg>

namespace ams {

    namespace fastboot {

        class FastbootGadget : public xusb::Gadget {
            public:
            FastbootGadget(uint8_t *download_buffer, size_t download_buffer_size);

            enum class ResponseDisposition {
                ReadHostCommand,

                /* After this response completes, begin receive data phase. */
                Download,
                /* After this response completes, send an empty OKAY response with
                 * ReadHostCommand disposition. */
                Okay,
                /* After this response completes, call impl.Continue(). */
                Continue,
            
                Reboot,
                Chainload,
            } response_disposition;

            enum class ResponseToken {
                OKAY,
                INFO,
                FAIL,
                DATA,
            };

            fastboot_return Run();

            Result SendResponse(ResponseDisposition disposition, ResponseToken token, const char *fmt, ...);
            Result PrepareDownload(size_t size);

            size_t GetMaxDownloadSize() const;
            size_t GetLastDownloadSize() const;
            void *GetLastDownloadBuffer() const;

            private:
            void FormatResponse(ResponseToken token, const char *fmt, std::va_list args);
            Result ReadHostCommand();
            Result QueueTRBsForReceive();

            FastbootImpl impl;
        
            enum class State {
                Invalid,
                WaitingForHostCommand,
                SendingResponse,
                DataPhaseReceive,
                DataPhaseTransmit,
                Continuing,

                Exit,
                UsbError,
                Reboot,
                Chainload,
            } state;

            Result usb_error;

            uint8_t *download_buffer;
            size_t download_buffer_size;
            size_t download_head;
            size_t download_size;
            bool download_needs_zlp;
            int download_active_trbs;
        
            char command_buffer[65];
            char response_buffer[65];
        
            xusb::TRB *last_trb = nullptr;
        
            private:
            /* xusb::Gadget implementation */
            Result ProcessTransferEventImpl(xusb::TransferEventTRB *event, xusb::TRBBorrow transfer);
            virtual void ProcessTransferEvent(xusb::TransferEventTRB *event, xusb::TRBBorrow transfer) override;
            virtual Result HandleSetupRequest(usb::SetupPacket &packet) override;
            virtual Result GetDeviceDescriptor(uint8_t index, const usb::DeviceDescriptor **descriptor, uint16_t *length) override;
            virtual Result GetDeviceQualifierDescriptor(uint8_t index, const usb::DeviceQualifierDescriptor **descriptor, uint16_t *length) override;
            virtual Result GetConfigurationDescriptor(uint8_t index, const usb::ConfigurationDescriptor **descriptor, uint16_t *length) override;
            virtual Result GetBOSDescriptor(const usb::BOSDescriptor **descriptor, uint16_t *length) override;
            virtual Result GetStringDescriptor(uint8_t index, uint16_t language, const usb::CommonDescriptor **descriptor, uint16_t *length) override;
            virtual Result SetConfiguration(uint16_t configuration) override;
            virtual Result Deconfigure() override;
            virtual void PostConfigure() override;
        };
    
    } // namespace fastboot

} // namespace ams
