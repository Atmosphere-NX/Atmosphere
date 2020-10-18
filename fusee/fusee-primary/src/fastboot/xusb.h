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

#include "usb_types.h"
#include "xusb_trb.h"
#include "xusb_endpoint.h"

#include<vapours/results/xusb_gadget_results.hpp>

#include<stddef.h>

namespace ams {

    namespace xusb {

        class Gadget {
            public:
            virtual Result HandleSetupRequest(usb::SetupPacket &packet);
            virtual Result GetDeviceDescriptor(uint8_t index, const usb::DeviceDescriptor **descriptor, uint16_t *length) = 0;
            virtual Result GetDeviceQualifierDescriptor(uint8_t index, const usb::DeviceQualifierDescriptor **descriptor, uint16_t *length) = 0; 
            virtual Result GetConfigurationDescriptor(uint8_t index, const usb::ConfigurationDescriptor **descriptor, uint16_t *length) = 0;
            virtual Result GetBOSDescriptor(const usb::BOSDescriptor **descriptor, uint16_t *length) = 0;
            virtual Result GetStringDescriptor(uint8_t index, uint16_t language, const usb::CommonDescriptor **descriptor, uint16_t *length) = 0;

            /* It is the implementor's responsibility to configure endpoints before this returns. */
            virtual Result SetConfiguration(uint16_t configuration) = 0;
            virtual void PostConfigure() = 0;
            virtual Result Deconfigure() = 0;

            virtual void ProcessTransferEvent(TransferEventTRB *event, TRBBorrow transfer) = 0;
        };
    
        void Initialize();
        void EnableDevice(Gadget &gadget);
        void Process();
        void Finalize();

        Gadget *GetCurrentGadget();
    
    } // namespace xusb

} // namespace ams
