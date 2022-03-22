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
#include <vapours.hpp>
#include <stratosphere/usb/usb_limits.hpp>
#include <stratosphere/usb/usb_types.hpp>

namespace ams::usb {

    constexpr inline u8 InterfaceNumberAuto    = DsLimitMaxInterfacesPerConfigurationCount;
    constexpr inline u8 EndpointAddressAutoIn  = UsbEndpointAddressMask_DirDevicetoHost;
    constexpr inline u8 EndpointAddressAutoOut = UsbEndpointAddressMask_DirHostToDevice;

    enum UrbStatus {
        UrbStatus_Invalid   = 0,
        UrbStatus_Pending   = 1,
        UrbStatus_Running   = 2,
        UrbStatus_Finished  = 3,
        UrbStatus_Cancelled = 4,
        UrbStatus_Failed    = 5,
    };

    struct UrbReport {
        struct Report {
            u32 id;
            u32 requested_size;
            u32 transferred_size;
            UrbStatus status;
        } reports[DsLimitRingSize];
        u32 count;
    };

    enum DsString {
        DsString_Max = 0x20,
    };

    struct DsVidPidBcd {
        uint16_t idVendor;
        uint16_t idProduct;
        uint16_t bcdDevice;

        char manufacturer[DsString_Max];
        char product[DsString_Max];
        char serial_number[DsString_Max];
    };

}
