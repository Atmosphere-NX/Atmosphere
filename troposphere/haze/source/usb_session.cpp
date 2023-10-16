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
#include <haze.hpp>

namespace haze {

    namespace {

        constexpr const u32 DefaultInterfaceNumber = 0;

    }

    Result UsbSession::Initialize1x(const UsbCommsInterfaceInfo *info) {
        struct usb_interface_descriptor interface_descriptor = {
            .bLength            = USB_DT_INTERFACE_SIZE,
            .bDescriptorType    = USB_DT_INTERFACE,
            .bInterfaceNumber   = DefaultInterfaceNumber,
            .bInterfaceClass    = info->bInterfaceClass,
            .bInterfaceSubClass = info->bInterfaceSubClass,
            .bInterfaceProtocol = info->bInterfaceProtocol,
        };

        struct usb_endpoint_descriptor endpoint_descriptor_in = {
            .bLength          = USB_DT_ENDPOINT_SIZE,
            .bDescriptorType  = USB_DT_ENDPOINT,
            .bEndpointAddress = USB_ENDPOINT_IN,
            .bmAttributes     = USB_TRANSFER_TYPE_BULK,
            .wMaxPacketSize   = PtpUsbBulkHighSpeedMaxPacketLength,
        };

        struct usb_endpoint_descriptor endpoint_descriptor_out = {
            .bLength          = USB_DT_ENDPOINT_SIZE,
            .bDescriptorType  = USB_DT_ENDPOINT,
            .bEndpointAddress = USB_ENDPOINT_OUT,
            .bmAttributes     = USB_TRANSFER_TYPE_BULK,
            .wMaxPacketSize   = PtpUsbBulkHighSpeedMaxPacketLength,
        };

        struct usb_endpoint_descriptor endpoint_descriptor_interrupt = {
            .bLength          = USB_DT_ENDPOINT_SIZE,
            .bDescriptorType  = USB_DT_ENDPOINT,
            .bEndpointAddress = USB_ENDPOINT_IN,
            .bmAttributes     = USB_TRANSFER_TYPE_INTERRUPT,
            .wMaxPacketSize   = 0x18,
            .bInterval        = 0x4,
        };

        /* Set up interface. */
        R_TRY(usbDsGetDsInterface(std::addressof(m_interface), std::addressof(interface_descriptor), "usb"));

        /* Set up endpoints. */
        R_TRY(usbDsInterface_GetDsEndpoint(m_interface, std::addressof(m_endpoints[UsbSessionEndpoint_Write]), std::addressof(endpoint_descriptor_in)));
        R_TRY(usbDsInterface_GetDsEndpoint(m_interface, std::addressof(m_endpoints[UsbSessionEndpoint_Read]), std::addressof(endpoint_descriptor_out)));
        R_TRY(usbDsInterface_GetDsEndpoint(m_interface, std::addressof(m_endpoints[UsbSessionEndpoint_Interrupt]), std::addressof(endpoint_descriptor_interrupt)));

        R_RETURN(usbDsInterface_EnableInterface(m_interface));
    }

    Result UsbSession::Initialize5x(const UsbCommsInterfaceInfo *info) {
        struct usb_interface_descriptor interface_descriptor = {
            .bLength            = USB_DT_INTERFACE_SIZE,
            .bDescriptorType    = USB_DT_INTERFACE,
            .bInterfaceNumber   = DefaultInterfaceNumber,
            .bNumEndpoints      = 3,
            .bInterfaceClass    = info->bInterfaceClass,
            .bInterfaceSubClass = info->bInterfaceSubClass,
            .bInterfaceProtocol = info->bInterfaceProtocol,
        };

        struct usb_endpoint_descriptor endpoint_descriptor_in = {
            .bLength          = USB_DT_ENDPOINT_SIZE,
            .bDescriptorType  = USB_DT_ENDPOINT,
            .bEndpointAddress = USB_ENDPOINT_IN,
            .bmAttributes     = USB_TRANSFER_TYPE_BULK,
            .wMaxPacketSize   = PtpUsbBulkHighSpeedMaxPacketLength,
        };

        struct usb_endpoint_descriptor endpoint_descriptor_out = {
            .bLength          = USB_DT_ENDPOINT_SIZE,
            .bDescriptorType  = USB_DT_ENDPOINT,
            .bEndpointAddress = USB_ENDPOINT_OUT,
            .bmAttributes     = USB_TRANSFER_TYPE_BULK,
            .wMaxPacketSize   = PtpUsbBulkHighSpeedMaxPacketLength,
        };

        struct usb_endpoint_descriptor endpoint_descriptor_interrupt = {
            .bLength          = USB_DT_ENDPOINT_SIZE,
            .bDescriptorType  = USB_DT_ENDPOINT,
            .bEndpointAddress = USB_ENDPOINT_IN,
            .bmAttributes     = USB_TRANSFER_TYPE_INTERRUPT,
            .wMaxPacketSize   = 0x18,
            .bInterval        = 0x6,
        };

        struct usb_ss_endpoint_companion_descriptor endpoint_companion = {
            .bLength           = sizeof(struct usb_ss_endpoint_companion_descriptor),
            .bDescriptorType   = USB_DT_SS_ENDPOINT_COMPANION,
            .bMaxBurst         = 0x0f,
            .bmAttributes      = 0x00,
            .wBytesPerInterval = 0x00,
        };

        struct usb_ss_endpoint_companion_descriptor endpoint_companion_interrupt = {
            .bLength           = sizeof(struct usb_ss_endpoint_companion_descriptor),
            .bDescriptorType   = USB_DT_SS_ENDPOINT_COMPANION,
            .bMaxBurst         = 0x00,
            .bmAttributes      = 0x00,
            .wBytesPerInterval = 0x00,
        };

        R_TRY(usbDsRegisterInterface(std::addressof(m_interface)));

        u8 iInterface;
        R_TRY(usbDsAddUsbStringDescriptor(std::addressof(iInterface), "MTP"));

        interface_descriptor.bInterfaceNumber = m_interface->interface_index;
        interface_descriptor.iInterface = iInterface;
        endpoint_descriptor_in.bEndpointAddress += interface_descriptor.bInterfaceNumber + 1;
        endpoint_descriptor_out.bEndpointAddress += interface_descriptor.bInterfaceNumber + 1;
        endpoint_descriptor_interrupt.bEndpointAddress += interface_descriptor.bInterfaceNumber + 2;

        /* High speed config. */
        R_TRY(usbDsInterface_AppendConfigurationData(m_interface, UsbDeviceSpeed_High, std::addressof(interface_descriptor), USB_DT_INTERFACE_SIZE));
        R_TRY(usbDsInterface_AppendConfigurationData(m_interface, UsbDeviceSpeed_High, std::addressof(endpoint_descriptor_in), USB_DT_ENDPOINT_SIZE));
        R_TRY(usbDsInterface_AppendConfigurationData(m_interface, UsbDeviceSpeed_High, std::addressof(endpoint_descriptor_out), USB_DT_ENDPOINT_SIZE));
        R_TRY(usbDsInterface_AppendConfigurationData(m_interface, UsbDeviceSpeed_High, std::addressof(endpoint_descriptor_interrupt), USB_DT_ENDPOINT_SIZE));

        /* Super speed config. */
        endpoint_descriptor_in.wMaxPacketSize  = PtpUsbBulkSuperSpeedMaxPacketLength;
        endpoint_descriptor_out.wMaxPacketSize = PtpUsbBulkSuperSpeedMaxPacketLength;
        R_TRY(usbDsInterface_AppendConfigurationData(m_interface, UsbDeviceSpeed_Super, std::addressof(interface_descriptor), USB_DT_INTERFACE_SIZE));
        R_TRY(usbDsInterface_AppendConfigurationData(m_interface, UsbDeviceSpeed_Super, std::addressof(endpoint_descriptor_in), USB_DT_ENDPOINT_SIZE));
        R_TRY(usbDsInterface_AppendConfigurationData(m_interface, UsbDeviceSpeed_Super, std::addressof(endpoint_companion), USB_DT_SS_ENDPOINT_COMPANION_SIZE));
        R_TRY(usbDsInterface_AppendConfigurationData(m_interface, UsbDeviceSpeed_Super, std::addressof(endpoint_descriptor_out), USB_DT_ENDPOINT_SIZE));
        R_TRY(usbDsInterface_AppendConfigurationData(m_interface, UsbDeviceSpeed_Super, std::addressof(endpoint_companion), USB_DT_SS_ENDPOINT_COMPANION_SIZE));
        R_TRY(usbDsInterface_AppendConfigurationData(m_interface, UsbDeviceSpeed_Super, std::addressof(endpoint_descriptor_interrupt), USB_DT_ENDPOINT_SIZE));
        R_TRY(usbDsInterface_AppendConfigurationData(m_interface, UsbDeviceSpeed_Super, std::addressof(endpoint_companion_interrupt), USB_DT_SS_ENDPOINT_COMPANION_SIZE));

        /* Set up endpoints. */
        R_TRY(usbDsInterface_RegisterEndpoint(m_interface, std::addressof(m_endpoints[UsbSessionEndpoint_Write]), endpoint_descriptor_in.bEndpointAddress));
        R_TRY(usbDsInterface_RegisterEndpoint(m_interface, std::addressof(m_endpoints[UsbSessionEndpoint_Read]), endpoint_descriptor_out.bEndpointAddress));
        R_TRY(usbDsInterface_RegisterEndpoint(m_interface, std::addressof(m_endpoints[UsbSessionEndpoint_Interrupt]), endpoint_descriptor_interrupt.bEndpointAddress));

        R_RETURN(usbDsInterface_EnableInterface(m_interface));
    }

    Result UsbSession::Initialize(const UsbCommsInterfaceInfo *info, u16 id_vendor, u16 id_product) {
        R_TRY(usbDsInitialize());

        if (hosversionAtLeast(5, 0, 0)) {
            /* Report language as US English. */
            static const u16 supported_langs[1] = { 0x0409 };
            R_TRY(usbDsAddUsbLanguageStringDescriptor(nullptr, supported_langs, util::size(supported_langs)));

            /* Report strings. */
            u8 iManufacturer, iProduct, iSerialNumber;
            R_TRY(usbDsAddUsbStringDescriptor(std::addressof(iManufacturer), "Nintendo"));
            R_TRY(usbDsAddUsbStringDescriptor(std::addressof(iProduct), "Nintendo Switch"));
            R_TRY(usbDsAddUsbStringDescriptor(std::addressof(iSerialNumber), GetSerialNumber()));

            /* Send device descriptors */
            struct usb_device_descriptor device_descriptor = {
                .bLength            = USB_DT_DEVICE_SIZE,
                .bDescriptorType    = USB_DT_DEVICE,
                .bcdUSB             = 0x0200,
                .bDeviceClass       = 0x00,
                .bDeviceSubClass    = 0x00,
                .bDeviceProtocol    = 0x00,
                .bMaxPacketSize0    = 0x40,
                .idVendor           = id_vendor,
                .idProduct          = id_product,
                .bcdDevice          = 0x0100,
                .iManufacturer      = iManufacturer,
                .iProduct           = iProduct,
                .iSerialNumber      = iSerialNumber,
                .bNumConfigurations = 0x01
            };
            R_TRY(usbDsSetUsbDeviceDescriptor(UsbDeviceSpeed_High, std::addressof(device_descriptor)));

            device_descriptor.bcdUSB = 0x0300;
            device_descriptor.bMaxPacketSize0 = 0x09;
            R_TRY(usbDsSetUsbDeviceDescriptor(UsbDeviceSpeed_Super, std::addressof(device_descriptor)));

            /* Binary Object Store */
            u8 bos[0x16] = {
                0x05,       /* .bLength */
                USB_DT_BOS, /* .bDescriptorType */
                0x16, 0x00, /* .wTotalLength */
                0x02,       /* .bNumDeviceCaps */

                /* USB 2.0 */
                0x07,                     /* .bLength */
                USB_DT_DEVICE_CAPABILITY, /* .bDescriptorType */
                0x02,                     /* .bDevCapabilityType */
                0x02, 0x00, 0x00, 0x00,   /* .bmAttributes */

                /* USB 3.0 */
                0x0a,                     /* .bLength */
                USB_DT_DEVICE_CAPABILITY, /* .bDescriptorType */
                0x03,                     /* .bDevCapabilityType */
                0x00,                     /* .bmAttributes */
                0x0c, 0x00,               /* .wSpeedSupported */
                0x03,                     /* .bFunctionalitySupport */
                0x00,                     /* .bU1DevExitLat */
                0x00, 0x00                /* .bU2DevExitLat */
            };
            R_TRY(usbDsSetBinaryObjectStore(bos, sizeof(bos)));
        }

        if (hosversionAtLeast(5, 0, 0)) {
            R_TRY(this->Initialize5x(info));
            R_TRY(usbDsEnable());
        } else {
            R_TRY(this->Initialize1x(info));
        }

        R_SUCCEED();
    }

    void UsbSession::Finalize() {
        usbDsExit();
    }

    bool UsbSession::GetConfigured() const {
        UsbState usb_state;

        HAZE_R_ABORT_UNLESS(usbDsGetState(std::addressof(usb_state)));

        return usb_state == UsbState_Configured;
    }

    Event *UsbSession::GetCompletionEvent(UsbSessionEndpoint ep) const {
        return std::addressof(m_endpoints[ep]->CompletionEvent);
    }

    Result UsbSession::TransferAsync(UsbSessionEndpoint ep, void *buffer, u32 size, u32 *out_urb_id) {
        R_RETURN(usbDsEndpoint_PostBufferAsync(m_endpoints[ep], buffer, size, out_urb_id));
    }

    Result UsbSession::GetTransferResult(UsbSessionEndpoint ep, u32 urb_id, u32 *out_transferred_size) {
        UsbDsReportData report_data;

        R_TRY(eventClear(std::addressof(m_endpoints[ep]->CompletionEvent)));
        R_TRY(usbDsEndpoint_GetReportData(m_endpoints[ep], std::addressof(report_data)));
        R_TRY(usbDsParseReportData(std::addressof(report_data), urb_id, nullptr, out_transferred_size));

        R_SUCCEED();
    }

}
