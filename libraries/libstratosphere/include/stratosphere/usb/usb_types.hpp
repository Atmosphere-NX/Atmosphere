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

namespace ams::usb {

    constexpr ALWAYS_INLINE bool IsDmaAligned(u64 address) {
        return util::IsAligned(address, static_cast<u64>(HwLimitDmaBufferAlignmentSize));
    }

    enum ComplexId {
        ComplexId_Tegra21x = 2,
    };

    enum UsbDescriptorType {
        UsbDescriptorType_Device                  =  1,
        UsbDescriptorType_Config                  =  2,
        UsbDescriptorType_String                  =  3,
        UsbDescriptorType_Interface               =  4,
        UsbDescriptorType_Endpoint                =  5,
        UsbDescriptorType_DeviceQualifier         =  6,
        UsbDescriptorType_OtherSpeedConfig        =  7,
        UsbDescriptorType_InterfacePower          =  8,
        UsbDescriptorType_Otg                     =  9,
        UsbDescriptorType_Debug                   = 10,
        UsbDescriptorType_InterfaceAssociation    = 11,
        UsbDescriptorType_Bos                     = 15,
        UsbDescriptorType_DeviceCapability        = 16,

        UsbDescriptorType_Hid                     = 33,
        UsbDescriptorType_Report                  = 34,
        UsbDescriptorType_Physical                = 35,

        UsbDescriptorType_Hub                     = 41,

        UsbDescriptorType_EndpointCompanion       = 48,
        UsbDescriptorType_IsocEndpointCompanion   = 49,
    };

    struct UsbDescriptorHeader {
        uint8_t bLength;
        uint8_t bDescriptorType;
    } PACKED;

    struct UsbInterfaceDescriptor {
        uint8_t  bLength;
        uint8_t  bDescriptorType;
        uint8_t  bInterfaceNumber;
        uint8_t  bAlternateSetting;
        uint8_t  bNumEndpoints;
        uint8_t  bInterfaceClass;
        uint8_t  bInterfaceSubClass;
        uint8_t  bInterfaceProtocol;
        uint8_t  iInterface;
    } PACKED;

    struct UsbEndpointDescriptor {
        uint8_t  bLength;
        uint8_t  bDescriptorType;
        uint8_t  bEndpointAddress;
        uint8_t  bmAttributes;
        uint16_t wMaxPacketSize;
        uint8_t  bInterval;
    } PACKED;

    struct UsbDeviceDescriptor {
        uint8_t  bLength;
        uint8_t  bDescriptorType;
        uint16_t bcdUSB;
        uint8_t  bDeviceClass;
        uint8_t  bDeviceSubClass;
        uint8_t  bDeviceProtocol;
        uint8_t  bMaxPacketSize0;
        uint16_t idVendor;
        uint16_t idProduct;
        uint16_t bcdDevice;
        uint8_t  iManufacturer;
        uint8_t  iProduct;
        uint8_t  iSerialNumber;
        uint8_t  bNumConfigurations;
    } PACKED;

    struct UsbConfigDescriptor {
        uint8_t  bLength;
        uint8_t  bDescriptorType;
        uint16_t wTotalLength;
        uint8_t  bNumInterfaces;
        uint8_t  bConfigurationValue;
        uint8_t  iConfiguration;
        uint8_t  bmAttributes;
        uint8_t  bMaxPower;
    } PACKED;

    struct UsbEndpointCompanionDescriptor {
        uint8_t  bLength;
        uint8_t  bDescriptorType;
        uint8_t  bMaxBurst;
        uint8_t  bmAttributes;
        uint16_t wBytesPerInterval;
    } PACKED;

    struct UsbStringDescriptor {
        uint8_t  bLength;
        uint8_t  bDescriptorType;
        uint16_t wData[DsLimitMaxNameSize];
    } PACKED;

    struct UsbCtrlRequest {
        uint8_t  bmRequestType;
        uint8_t  bRequest;
        uint16_t wValue;
        uint16_t wIndex;
        uint16_t wLength;
    } PACKED;

    enum UsbState {
        UsbState_Detached   = 0,
        UsbState_Attached   = 1,
        UsbState_Powered    = 2,
        UsbState_Default    = 3,
        UsbState_Address    = 4,
        UsbState_Configured = 5,
        UsbState_Suspended  = 6,
    };

    enum UsbDescriptorSize {
        UsbDescriptorSize_Interface         = sizeof(UsbInterfaceDescriptor),
        UsbDescriptorSize_Endpoint          = sizeof(UsbEndpointDescriptor),
        UsbDescriptorSize_Device            = sizeof(UsbDeviceDescriptor),
        UsbDescriptorSize_Config            = sizeof(UsbConfigDescriptor),
        UsbDescriptorSize_EndpointCompanion = sizeof(UsbEndpointCompanionDescriptor),
    };

    enum UsbDeviceSpeed {
        UsbDeviceSpeed_Invalid   = 0,
        UsbDeviceSpeed_Low       = 1,
        UsbDeviceSpeed_Full      = 2,
        UsbDeviceSpeed_High      = 3,
        UsbDeviceSpeed_Super     = 4,
        UsbDeviceSpeed_SuperPlus = 5,
    };


    enum UsbEndpointAddressMask {
        UsbEndpointAddressMask_EndpointNumber  = (0xF << 0),

        UsbEndpointAddressMask_Dir             = (0x1 << 7),

        UsbEndpointAddressMask_DirHostToDevice = (0x0 << 7),
        UsbEndpointAddressMask_DirDevicetoHost = (0x1 << 7),
    };

    enum UsbEndpointAttributeMask {
        UsbEndpointAttributeMask_XferType        = (0x3 << 0),

        UsbEndpointAttributeMask_XferTypeControl = (0x0 << 0),
        UsbEndpointAttributeMask_XferTypeIsoc    = (0x1 << 0),
        UsbEndpointAttributeMask_XferTypeBulk    = (0x2 << 0),
        UsbEndpointAttributeMask_XferTypeInt     = (0x3 << 0),
    };

    enum UsbEndpointDirection {
        UsbEndpointDirection_Invalid  = 0,
        UsbEndpointDirection_ToDevice = 1,
        UsbEndpointDirection_ToHost   = 2,
        UsbEndpointDirection_Control  = 3,
    };

    constexpr inline u8 UsbGetEndpointNumber(const UsbEndpointDescriptor *desc) {
        return desc->bEndpointAddress & UsbEndpointAddressMask_EndpointNumber;
    }

    constexpr inline bool UsbEndpointIsHostToDevice(const UsbEndpointDescriptor *desc) {
        return (desc->bEndpointAddress & UsbEndpointAddressMask_Dir) == UsbEndpointAddressMask_DirHostToDevice;
    }

    constexpr inline bool UsbEndpointIsDeviceToHost(const UsbEndpointDescriptor *desc) {
        return (desc->bEndpointAddress & UsbEndpointAddressMask_Dir) == UsbEndpointAddressMask_DirDevicetoHost;
    }

    constexpr inline u8 UsbGetEndpointAddress(u8 number, UsbEndpointDirection dir) {
        u8 val = static_cast<u8>(number & UsbEndpointAddressMask_EndpointNumber);
        if (dir == UsbEndpointDirection_ToHost) {
            val |= UsbEndpointAddressMask_DirDevicetoHost;
        } else {
            val |= UsbEndpointAddressMask_DirHostToDevice;
        }
        return val;
    }

    constexpr inline UsbEndpointDirection GetUsbEndpointDirection(const UsbEndpointDescriptor *desc) {
        if (UsbEndpointIsDeviceToHost(desc)) {
            return UsbEndpointDirection_ToHost;
        } else {
            return UsbEndpointDirection_ToDevice;
        }
    }

    constexpr inline bool UsbEndpointIsValid(const UsbEndpointDescriptor *desc) {
        return desc != nullptr && desc->bLength >= UsbDescriptorSize_Endpoint && desc->bEndpointAddress != 0;
    }

    constexpr inline void UsbMarkEndpointInvalid(UsbEndpointDescriptor *desc) {
        desc->bLength          = 0;
        desc->bEndpointAddress = 0;
    }

}
