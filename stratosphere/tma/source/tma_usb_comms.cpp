/*
 * Copyright (c) 2018 Atmosphère-NX
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

#include "tma_usb_comms.hpp"

/* TODO: Is this actually allowed? */
#define ATMOSPHERE_INTERFACE_PROTOCOL 0xFC

static std::atomic<bool> g_initialized = false;
static UsbDsInterface *g_interface;
static UsbDsEndpoint *g_endpoint_in, *g_endpoint_out;

/* USB State Change Tracking. */
static HosThread g_state_change_thread;
static WaitableManagerBase *g_state_change_manager = nullptr;
static void (*g_state_change_callback)(void *arg, u32 state);
static void *g_state_change_arg;

/* USB Send/Receive mutexes. */
static HosMutex g_send_mutex;
static HosMutex g_recv_mutex;

/* Static arrays to do USB DMA into. */
static constexpr size_t DmaBufferAlign = 0x1000;
static constexpr size_t HeaderBufferSize = DmaBufferAlign;
static constexpr size_t DataBufferSize = 0x18000;
static __attribute__((aligned(DmaBufferAlign))) u8 g_header_buffer[HeaderBufferSize];
static __attribute__((aligned(DmaBufferAlign))) u8 g_recv_data_buf[DataBufferSize];
static __attribute__((aligned(DmaBufferAlign))) u8 g_send_data_buf[DataBufferSize];

/* Taken from libnx usb comms. */
static Result _usbCommsInterfaceInit1x()
{
    Result rc = 0;

    struct usb_interface_descriptor interface_descriptor = {
        .bLength = USB_DT_INTERFACE_SIZE,
        .bDescriptorType = USB_DT_INTERFACE,
        .bInterfaceNumber = 4,
        .bInterfaceClass = USB_CLASS_VENDOR_SPEC,
        .bInterfaceSubClass = USB_CLASS_VENDOR_SPEC,
        .bInterfaceProtocol = ATMOSPHERE_INTERFACE_PROTOCOL,
    };

    struct usb_endpoint_descriptor endpoint_descriptor_in = {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = USB_ENDPOINT_IN,
        .bmAttributes = USB_TRANSFER_TYPE_BULK,
        .wMaxPacketSize = 0x200,
    };

    struct usb_endpoint_descriptor endpoint_descriptor_out = {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = USB_ENDPOINT_OUT,
        .bmAttributes = USB_TRANSFER_TYPE_BULK,
        .wMaxPacketSize = 0x200,
    };

    if (R_FAILED(rc)) return rc;

    //Setup interface.
    rc = usbDsGetDsInterface(&g_interface, &interface_descriptor, "usb");
    if (R_FAILED(rc)) return rc;

    //Setup endpoints.
    rc = usbDsInterface_GetDsEndpoint(g_interface, &g_endpoint_in, &endpoint_descriptor_in);//device->host
    if (R_FAILED(rc)) return rc;

    rc = usbDsInterface_GetDsEndpoint(g_interface, &g_endpoint_out, &endpoint_descriptor_out);//host->device
    if (R_FAILED(rc)) return rc;

    return rc;
}

static Result _usbCommsInterfaceInit5x() {
    Result rc = 0;
    
    u8 iManufacturer, iProduct, iSerialNumber;
    static const u16 supported_langs[1] = {0x0409};
    // Send language descriptor
    rc = usbDsAddUsbLanguageStringDescriptor(NULL, supported_langs, sizeof(supported_langs)/sizeof(u16));
    // Send manufacturer
    if (R_SUCCEEDED(rc)) rc = usbDsAddUsbStringDescriptor(&iManufacturer, "Nintendo");
    // Send product
    if (R_SUCCEEDED(rc)) rc = usbDsAddUsbStringDescriptor(&iProduct, "Nintendo Switch");
    // Send serial number
    if (R_SUCCEEDED(rc)) rc = usbDsAddUsbStringDescriptor(&iSerialNumber, "SerialNumber");

    // Send device descriptors
    struct usb_device_descriptor device_descriptor = {
        .bLength = USB_DT_DEVICE_SIZE,
        .bDescriptorType = USB_DT_DEVICE,
        .bcdUSB = 0x0110,
        .bDeviceClass = 0x00,
        .bDeviceSubClass = 0x00,
        .bDeviceProtocol = 0x00,
        .bMaxPacketSize0 = 0x40,
        .idVendor = 0x057e,
        .idProduct = 0x3000,
        .bcdDevice = 0x0100,
        .iManufacturer = iManufacturer,
        .iProduct = iProduct,
        .iSerialNumber = iSerialNumber,
        .bNumConfigurations = 0x01
    };
    // Full Speed is USB 1.1
    if (R_SUCCEEDED(rc)) rc = usbDsSetUsbDeviceDescriptor(UsbDeviceSpeed_Full, &device_descriptor);
    
    // High Speed is USB 2.0
    device_descriptor.bcdUSB = 0x0200;
    if (R_SUCCEEDED(rc)) rc = usbDsSetUsbDeviceDescriptor(UsbDeviceSpeed_High, &device_descriptor);
    
    // Super Speed is USB 3.0
    device_descriptor.bcdUSB = 0x0300;
    // Upgrade packet size to 512
    device_descriptor.bMaxPacketSize0 = 0x09;
    if (R_SUCCEEDED(rc)) rc = usbDsSetUsbDeviceDescriptor(UsbDeviceSpeed_Super, &device_descriptor);
    
    // Define Binary Object Store
    u8 bos[0x16] = {
        0x05, // .bLength
        USB_DT_BOS, // .bDescriptorType
        0x16, 0x00, // .wTotalLength
        0x02, // .bNumDeviceCaps
        
        // USB 2.0
        0x07, // .bLength
        USB_DT_DEVICE_CAPABILITY, // .bDescriptorType
        0x02, // .bDevCapabilityType
        0x02, 0x00, 0x00, 0x00, // dev_capability_data
        
        // USB 3.0
        0x0A, // .bLength
        USB_DT_DEVICE_CAPABILITY, // .bDescriptorType
        0x03, // .bDevCapabilityType
        0x00, 0x0E, 0x00, 0x03, 0x00, 0x00, 0x00
    };
    if (R_SUCCEEDED(rc)) rc = usbDsSetBinaryObjectStore(bos, sizeof(bos));
    
    if (R_FAILED(rc)) return rc;
    
    struct usb_interface_descriptor interface_descriptor = {
        .bLength = USB_DT_INTERFACE_SIZE,
        .bDescriptorType = USB_DT_INTERFACE,
        .bInterfaceNumber = 4,
        .bNumEndpoints = 2,
        .bInterfaceClass = USB_CLASS_VENDOR_SPEC,
        .bInterfaceSubClass = USB_CLASS_VENDOR_SPEC,
        .bInterfaceProtocol = ATMOSPHERE_INTERFACE_PROTOCOL,
    };

    struct usb_endpoint_descriptor endpoint_descriptor_in = {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = USB_ENDPOINT_IN,
        .bmAttributes = USB_TRANSFER_TYPE_BULK,
        .wMaxPacketSize = 0x40,
    };

    struct usb_endpoint_descriptor endpoint_descriptor_out = {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = USB_ENDPOINT_OUT,
        .bmAttributes = USB_TRANSFER_TYPE_BULK,
        .wMaxPacketSize = 0x40,
    };
    
    struct usb_ss_endpoint_companion_descriptor endpoint_companion = {
        .bLength = sizeof(struct usb_ss_endpoint_companion_descriptor),
        .bDescriptorType = USB_DT_SS_ENDPOINT_COMPANION,
        .bMaxBurst = 0x0F,
        .bmAttributes = 0x00,
        .wBytesPerInterval = 0x00,
    };
    
    rc = usbDsRegisterInterface(&g_interface);
    if (R_FAILED(rc)) return rc;
    
    interface_descriptor.bInterfaceNumber = g_interface->interface_index;
    endpoint_descriptor_in.bEndpointAddress += interface_descriptor.bInterfaceNumber + 1;
    endpoint_descriptor_out.bEndpointAddress += interface_descriptor.bInterfaceNumber + 1;
    
    // Full Speed Config
    rc = usbDsInterface_AppendConfigurationData(g_interface, UsbDeviceSpeed_Full, &interface_descriptor, USB_DT_INTERFACE_SIZE);
    if (R_FAILED(rc)) return rc;
    rc = usbDsInterface_AppendConfigurationData(g_interface, UsbDeviceSpeed_Full, &endpoint_descriptor_in, USB_DT_ENDPOINT_SIZE);
    if (R_FAILED(rc)) return rc;
    rc = usbDsInterface_AppendConfigurationData(g_interface, UsbDeviceSpeed_Full, &endpoint_descriptor_out, USB_DT_ENDPOINT_SIZE);
    if (R_FAILED(rc)) return rc;
    
    // High Speed Config
    endpoint_descriptor_in.wMaxPacketSize = 0x200;
    endpoint_descriptor_out.wMaxPacketSize = 0x200;
    rc = usbDsInterface_AppendConfigurationData(g_interface, UsbDeviceSpeed_High, &interface_descriptor, USB_DT_INTERFACE_SIZE);
    if (R_FAILED(rc)) return rc;
    rc = usbDsInterface_AppendConfigurationData(g_interface, UsbDeviceSpeed_High, &endpoint_descriptor_in, USB_DT_ENDPOINT_SIZE);
    if (R_FAILED(rc)) return rc;
    rc = usbDsInterface_AppendConfigurationData(g_interface, UsbDeviceSpeed_High, &endpoint_descriptor_out, USB_DT_ENDPOINT_SIZE);
    if (R_FAILED(rc)) return rc;
    
    // Super Speed Config
    endpoint_descriptor_in.wMaxPacketSize = 0x400;
    endpoint_descriptor_out.wMaxPacketSize = 0x400;
    rc = usbDsInterface_AppendConfigurationData(g_interface, UsbDeviceSpeed_Super, &interface_descriptor, USB_DT_INTERFACE_SIZE);
    if (R_FAILED(rc)) return rc;
    rc = usbDsInterface_AppendConfigurationData(g_interface, UsbDeviceSpeed_Super, &endpoint_descriptor_in, USB_DT_ENDPOINT_SIZE);
    if (R_FAILED(rc)) return rc;
    rc = usbDsInterface_AppendConfigurationData(g_interface, UsbDeviceSpeed_Super, &endpoint_companion, USB_DT_SS_ENDPOINT_COMPANION_SIZE);
    if (R_FAILED(rc)) return rc;
    rc = usbDsInterface_AppendConfigurationData(g_interface, UsbDeviceSpeed_Super, &endpoint_descriptor_out, USB_DT_ENDPOINT_SIZE);
    if (R_FAILED(rc)) return rc;
    rc = usbDsInterface_AppendConfigurationData(g_interface, UsbDeviceSpeed_Super, &endpoint_companion, USB_DT_SS_ENDPOINT_COMPANION_SIZE);
    if (R_FAILED(rc)) return rc;
    
    //Setup endpoints.    
    rc = usbDsInterface_RegisterEndpoint(g_interface, &g_endpoint_in, endpoint_descriptor_in.bEndpointAddress);
    if (R_FAILED(rc)) return rc;
    
    rc = usbDsInterface_RegisterEndpoint(g_interface, &g_endpoint_out, endpoint_descriptor_out.bEndpointAddress);
    if (R_FAILED(rc)) return rc;
    
    return rc;
}


/* Actual function implementations. */
TmaConnResult TmaUsbComms::Initialize() {
    TmaConnResult res = TmaConnResult::Success;
    
    if (g_initialized) {
        std::abort();
    }
    
    Result rc = usbDsInitialize();
    
    /* Perform interface setup. */
    if (R_SUCCEEDED(rc)) {
        if (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) {
            rc = _usbCommsInterfaceInit5x();
        } else {
            rc = _usbCommsInterfaceInit1x();
        }
    }
    
    /* Start the state change thread. */
    if (R_SUCCEEDED(rc)) {
        rc = g_state_change_thread.Initialize(&TmaUsbComms::UsbStateChangeThreadFunc, nullptr, 0x4000, 38);
        if (R_SUCCEEDED(rc)) {
            rc = g_state_change_thread.Start();
        }
    }
    
    /* Enable USB communication. */
    if (R_SUCCEEDED(rc)) {
        rc = usbDsInterface_EnableInterface(g_interface);
    }
    if (R_SUCCEEDED(rc) && GetRuntimeFirmwareVersion() >= FirmwareVersion_500) {
        rc = usbDsEnable();
    }
    
    
    if (R_FAILED(rc)) {
        /* TODO: Should I not abort here? */
        std::abort();
    }
    
    g_initialized = true;
    
    return res;
}

TmaConnResult TmaUsbComms::Finalize() {
    Result rc = 0;
    /* We must have initialized before calling finalize. */
    if (!g_initialized) {
        std::abort();
    }
    
    /* Kill the state change thread. */
    g_state_change_manager->RequestStop();
    if (R_FAILED(g_state_change_thread.Join())) {
        std::abort();
    }
    
    CancelComms();
    if (R_SUCCEEDED(rc)) {
        usbDsExit();
    }
    
    g_state_change_callback = nullptr;
    g_interface = nullptr;
    g_endpoint_in = nullptr;
    g_endpoint_out = nullptr;
    g_initialized = false;
    
    return R_SUCCEEDED(rc) ? TmaConnResult::Success : TmaConnResult::ConnectionFailure;
}

void TmaUsbComms::CancelComms() {
    if (!g_initialized) {
        return;
    }
    
    usbDsEndpoint_Cancel(g_endpoint_in);
    usbDsEndpoint_Cancel(g_endpoint_out);
}

void TmaUsbComms::SetStateChangeCallback(void (*callback)(void *, u32), void *arg) {
    g_state_change_callback = callback;
    g_state_change_arg = arg;
}

Result TmaUsbComms::UsbXfer(UsbDsEndpoint *ep, size_t *out_xferd, void *buf, size_t size) {
    Result rc = 0;
    u32 urbId = 0;
    u32 total_xferd = 0;
    UsbDsReportData reportdata;
    
    if (size) {
        /* Start transfer. */
        rc = usbDsEndpoint_PostBufferAsync(ep, buf, size, &urbId);
        if (R_FAILED(rc)) return rc;
        
        /* Wait for transfer to complete. */
        eventWait(&ep->CompletionEvent, U64_MAX);
        eventClear(&ep->CompletionEvent);
        
        rc = usbDsEndpoint_GetReportData(ep, &reportdata);
        if (R_FAILED(rc)) return rc;

        rc = usbDsParseReportData(&reportdata, urbId, NULL, &total_xferd);
        if (R_FAILED(rc)) return rc;   
    }
    
    if (out_xferd) *out_xferd = total_xferd;
    
    return rc;
}

TmaConnResult TmaUsbComms::ReceivePacket(TmaPacket *packet) {
    std::scoped_lock<HosMutex> lk{g_recv_mutex};
    TmaConnResult res = TmaConnResult::Success;
    
    if (!g_initialized || packet == nullptr) {
        return TmaConnResult::GeneralFailure;
    }
    
    /* Read the header. */
    size_t read = 0;
    if (R_SUCCEEDED(UsbXfer(g_endpoint_out, &read, g_header_buffer, sizeof(TmaPacket::Header)))) {
        packet->CopyHeaderFrom(reinterpret_cast<TmaPacket::Header *>(g_header_buffer));
    } else {
        res = TmaConnResult::GeneralFailure;
    }
    
    /* Validate the read header data. */
    if (res == TmaConnResult::Success) {
        if (read != sizeof(TmaPacket::Header) || !packet->IsHeaderValid()) {
            res = TmaConnResult::GeneralFailure;
        }
    }
    
    /* Read the body! */
    if (res == TmaConnResult::Success) {
        const u32 body_len = packet->GetBodyLength();
        if (0 < body_len) {
            if (body_len <= sizeof(g_recv_data_buf)) {
                if (R_SUCCEEDED(UsbXfer(g_endpoint_out, &read, g_recv_data_buf, body_len))) {
                    if (read == body_len) {
                        res = packet->CopyBodyFrom(g_recv_data_buf, body_len);
                    } else {
                        res = TmaConnResult::GeneralFailure;
                    }
                }
            } else {
                res = TmaConnResult::GeneralFailure;
            }
        }
    }
    
    /* Validate the body. */
    if (res == TmaConnResult::Success) {
        if (!packet->IsBodyValid()) {
            res = TmaConnResult::GeneralFailure;
        }
    }
    
    if (res == TmaConnResult::Success) {
        packet->ClearOffset();
    }
    
    return res;
}

TmaConnResult TmaUsbComms::SendPacket(TmaPacket *packet) {
    std::scoped_lock<HosMutex> lk{g_send_mutex};
    TmaConnResult res = TmaConnResult::Success;
    
    if (!g_initialized || packet == nullptr) {
        return TmaConnResult::GeneralFailure;
    }
    
    /* Ensure our packets have the correct checksums. */
    packet->SetChecksums();
    
    /* Send the packet. */
    size_t written = 0;
    const u32 body_len = packet->GetBodyLength();
    if (body_len <= sizeof(g_send_data_buf)) {
        /* Copy header to send buffer. */
        packet->CopyHeaderTo(g_send_data_buf);
        
        /* Send the packet header. */
        if (R_SUCCEEDED(UsbXfer(g_endpoint_in, &written, g_send_data_buf, sizeof(TmaPacket::Header)))) {
            if (written == sizeof(TmaPacket::Header)) {
                res = TmaConnResult::Success;
            } else {
                res = TmaConnResult::GeneralFailure;
            }
        } else {
            res = TmaConnResult::GeneralFailure;
        }
        
        if (res == TmaConnResult::Success && 0 < body_len) {
            /* Copy body to send buffer. */
            packet->CopyBodyTo(g_send_data_buf);
            
            
            /* Send the packet body. */
            if (R_SUCCEEDED(UsbXfer(g_endpoint_in, &written, g_send_data_buf, body_len))) {
                if (written == body_len) {
                    res = TmaConnResult::Success;
                } else {
                    res = TmaConnResult::GeneralFailure;
                }
            } else {
                res = TmaConnResult::GeneralFailure;
            }
        }  
    } else {
        res = TmaConnResult::GeneralFailure;
    }

    return res;
}

void TmaUsbComms::UsbStateChangeThreadFunc(void *arg) {
    u32 state;
    g_state_change_manager = new WaitableManager(1);
    
    auto state_change_event = LoadReadOnlySystemEvent(usbDsGetStateChangeEvent()->revent, [&](u64 timeout) {
        if (R_SUCCEEDED(usbDsGetState(&state))) {
            if (g_state_change_callback != nullptr) {
                g_state_change_callback(g_state_change_arg, state);
            }
        }
        return 0;
    }, true);
    
    g_state_change_manager->AddWaitable(state_change_event);
    g_state_change_manager->Process();
    
    /* If we get here, we're exiting. */
    state_change_event->r_h = 0;
    delete g_state_change_manager;
    g_state_change_manager = nullptr;
    
    svcExitThread();
}