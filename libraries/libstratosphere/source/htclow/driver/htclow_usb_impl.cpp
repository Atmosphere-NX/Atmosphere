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
#include "htclow_usb_impl.hpp"
#include "htclow_driver_memory_management.hpp"

namespace ams::htclow::driver {

    namespace {

        /* TODO: Should we identify differently than Nintendo does? */
        /* It's kind of silly to identify as "NintendoSDK DevKit", but it's also kind of amusing. */
        /* TBD */

        constinit usb::UsbStringDescriptor LanguageStringDescriptor         = { 4, usb::UsbDescriptorType_String, {0x0409}};
        constinit usb::UsbStringDescriptor ManufacturerStringDescriptor     = {18, usb::UsbDescriptorType_String, {'N', 'i', 'n', 't', 'e', 'n', 'd', 'o'}};
        constinit usb::UsbStringDescriptor ProductStringFullSpeedDescriptor = {38, usb::UsbDescriptorType_String, {'N', 'i', 'n', 't', 'e', 'n', 'd', 'o', 'S', 'D', 'K', ' ', 'D', 'e', 'v', 'K', 'i', 't'}};
        constinit usb::UsbStringDescriptor SerialNumberStringDescriptor     = { 0, usb::UsbDescriptorType_String, {}};
        constinit usb::UsbStringDescriptor InterfaceStringDescriptor        = {16, usb::UsbDescriptorType_String, {'h', 't', 'c', ' ', 'u', 's', 'b'}};

        constinit usb::UsbDeviceDescriptor UsbDeviceDescriptorFullSpeed = {
            .bLength            = usb::UsbDescriptorSize_Device,
            .bDescriptorType    = usb::UsbDescriptorType_Device,
            .bcdUSB             = 0x0110,
            .bDeviceClass       = 0x00,
            .bDeviceSubClass    = 0x00,
            .bDeviceProtocol    = 0x00,
            .bMaxPacketSize0    = 0x40,
            .idVendor           = 0x057E,
            .idProduct          = 0x3005,
            .bcdDevice          = 0x0100,
            .iManufacturer      = 0x01,
            .iProduct           = 0x02,
            .iSerialNumber      = 0x03,
            .bNumConfigurations = 0x01,
        };

        constinit usb::UsbDeviceDescriptor UsbDeviceDescriptorHighSpeed = {
            .bLength            = usb::UsbDescriptorSize_Device,
            .bDescriptorType    = usb::UsbDescriptorType_Device,
            .bcdUSB             = 0x0200,
            .bDeviceClass       = 0x00,
            .bDeviceSubClass    = 0x00,
            .bDeviceProtocol    = 0x00,
            .bMaxPacketSize0    = 0x40,
            .idVendor           = 0x057E,
            .idProduct          = 0x3005,
            .bcdDevice          = 0x0100,
            .iManufacturer      = 0x01,
            .iProduct           = 0x02,
            .iSerialNumber      = 0x03,
            .bNumConfigurations = 0x01,
        };

        constinit usb::UsbDeviceDescriptor UsbDeviceDescriptorSuperSpeed = {
            .bLength            = usb::UsbDescriptorSize_Device,
            .bDescriptorType    = usb::UsbDescriptorType_Device,
            .bcdUSB             = 0x0300,
            .bDeviceClass       = 0x00,
            .bDeviceSubClass    = 0x00,
            .bDeviceProtocol    = 0x00,
            .bMaxPacketSize0    = 0x09,
            .idVendor           = 0x057E,
            .idProduct          = 0x3005,
            .bcdDevice          = 0x0100,
            .iManufacturer      = 0x01,
            .iProduct           = 0x02,
            .iSerialNumber      = 0x03,
            .bNumConfigurations = 0x01,
        };

        constinit u8 BinaryObjectStore[] = {
            0x05,
            usb::UsbDescriptorType_Bos,
            0x16, 0x00,
            0x02,

            0x07,
            usb::UsbDescriptorType_DeviceCapability,
            0x02,
            0x02, 0x00, 0x00, 0x00,

            0x0A,
            usb::UsbDescriptorType_DeviceCapability,
            0x03,
            0x00,
            0x0E, 0x00,
            0x03,
            0x00,
            0x00, 0x00,
        };

        constinit usb::UsbInterfaceDescriptor UsbInterfaceDescriptor = {
            .bLength            = usb::UsbDescriptorSize_Interface,
            .bDescriptorType    = usb::UsbDescriptorType_Interface,
            .bInterfaceNumber   = 0x00,
            .bAlternateSetting  = 0x00,
            .bNumEndpoints      = 0x02,
            .bInterfaceClass    = 0xFF,
            .bInterfaceSubClass = 0xFF,
            .bInterfaceProtocol = 0xFF,
            .iInterface         = 0x04,
        };

        constinit usb::UsbEndpointDescriptor UsbEndpointDescriptorsFullSpeed[2] = {
            {
                .bLength            = usb::UsbDescriptorSize_Endpoint,
                .bDescriptorType    = usb::UsbDescriptorType_Endpoint,
                .bEndpointAddress   = 0x81,
                .bmAttributes       = usb::UsbEndpointAttributeMask_XferTypeBulk,
                .wMaxPacketSize     = 0x40,
                .bInterval          = 0x00,
            },
            {
                .bLength            = usb::UsbDescriptorSize_Endpoint,
                .bDescriptorType    = usb::UsbDescriptorType_Endpoint,
                .bEndpointAddress   = 0x01,
                .bmAttributes       = usb::UsbEndpointAttributeMask_XferTypeBulk,
                .wMaxPacketSize     = 0x40,
                .bInterval          = 0x00,
            }
        };

        constinit usb::UsbEndpointDescriptor UsbEndpointDescriptorsHighSpeed[2] = {
            {
                .bLength            = usb::UsbDescriptorSize_Endpoint,
                .bDescriptorType    = usb::UsbDescriptorType_Endpoint,
                .bEndpointAddress   = 0x81,
                .bmAttributes       = usb::UsbEndpointAttributeMask_XferTypeBulk,
                .wMaxPacketSize     = 0x200,
                .bInterval          = 0x00,
            },
            {
                .bLength            = usb::UsbDescriptorSize_Endpoint,
                .bDescriptorType    = usb::UsbDescriptorType_Endpoint,
                .bEndpointAddress   = 0x01,
                .bmAttributes       = usb::UsbEndpointAttributeMask_XferTypeBulk,
                .wMaxPacketSize     = 0x200,
                .bInterval          = 0x00,
            }
        };

        constinit usb::UsbEndpointDescriptor UsbEndpointDescriptorsSuperSpeed[2] = {
            {
                .bLength            = usb::UsbDescriptorSize_Endpoint,
                .bDescriptorType    = usb::UsbDescriptorType_Endpoint,
                .bEndpointAddress   = 0x81,
                .bmAttributes       = usb::UsbEndpointAttributeMask_XferTypeBulk,
                .wMaxPacketSize     = 0x400,
                .bInterval          = 0x00,
            },
            {
                .bLength            = usb::UsbDescriptorSize_Endpoint,
                .bDescriptorType    = usb::UsbDescriptorType_Endpoint,
                .bEndpointAddress   = 0x01,
                .bmAttributes       = usb::UsbEndpointAttributeMask_XferTypeBulk,
                .wMaxPacketSize     = 0x400,
                .bInterval          = 0x00,
            }
        };

        constinit usb::UsbEndpointCompanionDescriptor UsbEndpointCompanionDescriptor = {
            .bLength            = usb::UsbDescriptorSize_EndpointCompanion,
            .bDescriptorType    = usb::UsbDescriptorType_EndpointCompanion,
            .bMaxBurst          = 0x0F,
            .bmAttributes       = 0x00,
            .wBytesPerInterval  = 0x0000,
        };

        constinit void *g_usb_receive_buffer = nullptr;
        constinit void *g_usb_send_buffer    = nullptr;

        constinit UsbAvailabilityChangeCallback g_availability_change_callback = nullptr;
        constinit void *g_availability_change_param = nullptr;

        constinit bool g_usb_interface_initialized = false;

        os::Event g_usb_break_event(os::EventClearMode_ManualClear);

        constinit os::ThreadType g_usb_indication_thread = {};

        constinit os::SdkMutex g_usb_driver_mutex;

        usb::DsClient    g_ds_client;
        usb::DsInterface g_ds_interface;
        usb::DsEndpoint  g_ds_endpoints[2];

        void InvokeAvailabilityChangeCallback(UsbAvailability availability) {
            if (g_availability_change_callback) {
                g_availability_change_callback(availability, g_availability_change_param);
            }
        }

        Result ConvertUsbDriverResult(Result result) {
            if (result.GetModule() == R_NAMESPACE_MODULE_ID(usb)) {
                if (usb::ResultResourceBusy::Includes(result)) {
                    R_THROW(htclow::ResultUsbDriverBusyError());
                } else if (usb::ResultMemAllocFailure::Includes(result)) {
                    R_THROW(htclow::ResultOutOfMemory());
                } else {
                    R_THROW(htclow::ResultUsbDriverUnknownError());
                }
            } else {
                R_RETURN(result);
            }
        }

        Result InitializeDsClient() {
            /* Initialize the client. */
            R_TRY(ConvertUsbDriverResult(g_ds_client.Initialize(usb::ComplexId_Tegra21x)));

            /* Clear device data. */
            R_ABORT_UNLESS(g_ds_client.ClearDeviceData());

            /* Add string descriptors. */
            u8 index;
            R_TRY(g_ds_client.AddUsbStringDescriptor(std::addressof(index), std::addressof(LanguageStringDescriptor)));
            R_TRY(g_ds_client.AddUsbStringDescriptor(std::addressof(index), std::addressof(ManufacturerStringDescriptor)));
            R_TRY(g_ds_client.AddUsbStringDescriptor(std::addressof(index), std::addressof(ProductStringFullSpeedDescriptor)));
            R_TRY(g_ds_client.AddUsbStringDescriptor(std::addressof(index), std::addressof(SerialNumberStringDescriptor)));
            R_TRY(g_ds_client.AddUsbStringDescriptor(std::addressof(index), std::addressof(InterfaceStringDescriptor)));

            /* Add device descriptors. */
            R_TRY(g_ds_client.SetUsbDeviceDescriptor(std::addressof(UsbDeviceDescriptorFullSpeed),  usb::UsbDeviceSpeed_Full));
            R_TRY(g_ds_client.SetUsbDeviceDescriptor(std::addressof(UsbDeviceDescriptorHighSpeed),  usb::UsbDeviceSpeed_High));
            R_TRY(g_ds_client.SetUsbDeviceDescriptor(std::addressof(UsbDeviceDescriptorSuperSpeed), usb::UsbDeviceSpeed_Super));

            /* Set binary object store. */
            R_TRY(g_ds_client.SetBinaryObjectStore(BinaryObjectStore, sizeof(BinaryObjectStore)));

            R_SUCCEED();
        }

        Result InitializeDsInterface() {
            /* Initialize the interface. */
            R_TRY(ConvertUsbDriverResult(g_ds_interface.Initialize(std::addressof(g_ds_client), 0)));

            /* Append the interface descriptors for all speeds. */
            R_TRY(g_ds_interface.AppendConfigurationData(usb::UsbDeviceSpeed_Full, std::addressof(UsbInterfaceDescriptor),             sizeof(usb::UsbInterfaceDescriptor)));
            R_TRY(g_ds_interface.AppendConfigurationData(usb::UsbDeviceSpeed_Full, std::addressof(UsbEndpointDescriptorsFullSpeed[0]), sizeof(usb::UsbEndpointDescriptor)));
            R_TRY(g_ds_interface.AppendConfigurationData(usb::UsbDeviceSpeed_Full, std::addressof(UsbEndpointDescriptorsFullSpeed[1]), sizeof(usb::UsbEndpointDescriptor)));

            R_TRY(g_ds_interface.AppendConfigurationData(usb::UsbDeviceSpeed_High, std::addressof(UsbInterfaceDescriptor),             sizeof(usb::UsbInterfaceDescriptor)));
            R_TRY(g_ds_interface.AppendConfigurationData(usb::UsbDeviceSpeed_High, std::addressof(UsbEndpointDescriptorsHighSpeed[0]), sizeof(usb::UsbEndpointDescriptor)));
            R_TRY(g_ds_interface.AppendConfigurationData(usb::UsbDeviceSpeed_High, std::addressof(UsbEndpointDescriptorsHighSpeed[1]), sizeof(usb::UsbEndpointDescriptor)));

            R_TRY(g_ds_interface.AppendConfigurationData(usb::UsbDeviceSpeed_Super, std::addressof(UsbInterfaceDescriptor),              sizeof(usb::UsbInterfaceDescriptor)));
            R_TRY(g_ds_interface.AppendConfigurationData(usb::UsbDeviceSpeed_Super, std::addressof(UsbEndpointDescriptorsSuperSpeed[0]), sizeof(usb::UsbEndpointDescriptor)));
            R_TRY(g_ds_interface.AppendConfigurationData(usb::UsbDeviceSpeed_Super, std::addressof(UsbEndpointCompanionDescriptor),      sizeof(usb::UsbEndpointCompanionDescriptor)));
            R_TRY(g_ds_interface.AppendConfigurationData(usb::UsbDeviceSpeed_Super, std::addressof(UsbEndpointDescriptorsSuperSpeed[1]), sizeof(usb::UsbEndpointDescriptor)));
            R_TRY(g_ds_interface.AppendConfigurationData(usb::UsbDeviceSpeed_Super, std::addressof(UsbEndpointCompanionDescriptor),      sizeof(usb::UsbEndpointCompanionDescriptor)));

            R_SUCCEED();
        }

        Result InitializeDsEndpoints() {
            R_TRY(g_ds_endpoints[0].Initialize(std::addressof(g_ds_interface), 0x81));
            R_TRY(g_ds_endpoints[1].Initialize(std::addressof(g_ds_interface), 0x01));
            R_SUCCEED();
        }

        void UsbIndicationThreadFunction(void *) {
            /* Get the state change event. */
            os::SystemEventType *state_change_event = g_ds_client.GetStateChangeEvent();

            /* Setup multi wait. */
            os::MultiWaitType multi_wait;
            os::InitializeMultiWait(std::addressof(multi_wait));

            /* Link multi wait holders. */
            os::MultiWaitHolderType state_change_holder;
            os::MultiWaitHolderType break_holder;
            os::InitializeMultiWaitHolder(std::addressof(state_change_holder), state_change_event);
            os::LinkMultiWaitHolder(std::addressof(multi_wait), std::addressof(state_change_holder));
            os::InitializeMultiWaitHolder(std::addressof(break_holder), g_usb_break_event.GetBase());
            os::LinkMultiWaitHolder(std::addressof(multi_wait), std::addressof(break_holder));

            /* Loop forever. */
            while (true) {
                /* If we should break, do so. */
                if (os::WaitAny(std::addressof(multi_wait)) == std::addressof(break_holder)) {
                    break;
                }

                /* Clear the state change event. */
                os::ClearSystemEvent(state_change_event);

                /* Get the new state. */
                usb::UsbState usb_state;
                R_ABORT_UNLESS(g_ds_client.GetState(std::addressof(usb_state)));

                switch (usb_state) {
                    case usb::UsbState_Detached:
                    case usb::UsbState_Suspended:
                        InvokeAvailabilityChangeCallback(UsbAvailability_Unavailable);
                        break;
                    case usb::UsbState_Configured:
                        InvokeAvailabilityChangeCallback(UsbAvailability_Available);
                        break;
                    default:
                        /* Nothing to do. */
                        break;
                }
            }

            /* Clear the break event. */
            g_usb_break_event.Clear();

            /* Unlink all holders. */
            os::UnlinkAllMultiWaitHolder(std::addressof(multi_wait));

            /* Finalize the multi wait/holders. */
            os::FinalizeMultiWaitHolder(std::addressof(break_holder));
            os::FinalizeMultiWaitHolder(std::addressof(state_change_holder));
            os::FinalizeMultiWait(std::addressof(multi_wait));
        }

    }

    void SetUsbAvailabilityChangeCallback(UsbAvailabilityChangeCallback callback, void *param) {
        g_availability_change_callback = callback;
        g_availability_change_param    = param;
    }

    void ClearUsbAvailabilityChangeCallback() {
        g_availability_change_callback = nullptr;
        g_availability_change_param    = nullptr;
    }

    Result InitializeUsbInterface() {
        /* Set the interface as initialized. */
        g_usb_interface_initialized = true;

        /* Get the dma buffers. */
        g_usb_receive_buffer = GetUsbReceiveBuffer();
        g_usb_send_buffer    = GetUsbSendBuffer();

        /* If we fail somewhere, finalize. */
        auto init_guard = SCOPE_GUARD { FinalizeUsbInterface(); };

        /* Get the serial number. */
        {
            settings::factory::SerialNumber serial_number;
            serial_number.str[0] = '\x00';

            if (R_FAILED(settings::factory::GetSerialNumber(std::addressof(serial_number))) || serial_number.str[0] == '\x00') {
                std::strcpy(serial_number.str, "Corrupted S/N");
            }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"

            u16 *dst = SerialNumberStringDescriptor.wData;
            u8  *src = reinterpret_cast<u8 *>(serial_number.str);
            u8 count = 0;

#pragma GCC diagnostic pop

            while (*src) {
                *dst++ = static_cast<u16>(*src++);
                ++count;
            }

            SerialNumberStringDescriptor.bLength = 2 + (2 * count);
        }

        /* Initialize the client. */
        R_TRY(InitializeDsClient());

        /* Initialize the interface. */
        R_TRY(InitializeDsInterface());

        /* Initialize the endpoints. */
        R_TRY(ConvertUsbDriverResult(InitializeDsEndpoints()));

        /* Create the indication thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(g_usb_indication_thread), &UsbIndicationThreadFunction, nullptr, GetUsbIndicationThreadStack(), UsbIndicationThreadStackSize, AMS_GET_SYSTEM_THREAD_PRIORITY(htc, HtclowUsbIndication)));

        /* Set the thread name. */
        os::SetThreadNamePointer(std::addressof(g_usb_indication_thread), AMS_GET_SYSTEM_THREAD_NAME(htc, HtclowUsbIndication));

        /* Start the indication thread. */
        os::StartThread(std::addressof(g_usb_indication_thread));

        /* Enable the usb device. */
        R_TRY(g_ds_client.EnableDevice());

        /* We succeeded! */
        init_guard.Cancel();
        R_SUCCEED();
    }

    void FinalizeUsbInterface() {
        g_usb_break_event.Signal();
        os::WaitThread(std::addressof(g_usb_indication_thread));
        os::DestroyThread(std::addressof(g_usb_indication_thread));
        static_cast<void>(g_ds_client.DisableDevice());
        static_cast<void>(g_ds_endpoints[1].Finalize());
        static_cast<void>(g_ds_endpoints[0].Finalize());
        static_cast<void>(g_ds_interface.Finalize());
        static_cast<void>(g_ds_client.Finalize());
        g_usb_interface_initialized = false;
    }

    Result SendUsb(int *out_transferred, const void *src, int src_size) {
        /* Acquire exclusive access to the driver. */
        std::scoped_lock lk(g_usb_driver_mutex);

        /* Check that we can send the data. */
        R_UNLESS(src_size <= static_cast<int>(UsbDmaBufferSize), htclow::ResultInvalidArgument());

        /* Copy the data to the dma buffer. */
        std::memcpy(g_usb_send_buffer, src, src_size);

        /* Transfer data. */
        u32 transferred;
        R_UNLESS(R_SUCCEEDED(g_ds_endpoints[0].PostBuffer(std::addressof(transferred), g_usb_send_buffer, src_size)), htclow::ResultUsbDriverSendError());
        R_UNLESS(transferred == static_cast<u32>(src_size),                                                           htclow::ResultUsbDriverSendError());

        /* Set output transferred size. */
        *out_transferred = src_size;
        R_SUCCEED();
    }

    Result ReceiveUsb(int *out_transferred, void *dst, int dst_size) {
        /* Check that we can send the data. */
        R_UNLESS(dst_size <= static_cast<int>(UsbDmaBufferSize), htclow::ResultInvalidArgument());

        /* Transfer data. */
        u32 transferred;
        R_UNLESS(R_SUCCEEDED(g_ds_endpoints[1].PostBuffer(std::addressof(transferred), g_usb_receive_buffer, dst_size)), htclow::ResultUsbDriverReceiveError());
        R_UNLESS(transferred == static_cast<u32>(dst_size),                                                              htclow::ResultUsbDriverReceiveError());

        /* Copy the data. */
        std::memcpy(dst, g_usb_receive_buffer, dst_size);

        /* Set output transferred size. */
        *out_transferred = dst_size;
        R_SUCCEED();
    }

    void CancelUsbSendReceive() {
        if (g_usb_interface_initialized) {
            static_cast<void>(g_ds_endpoints[0].Cancel());
            static_cast<void>(g_ds_endpoints[1].Cancel());
        }
    }

}
