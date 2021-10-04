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
#include <stratosphere/os/os_system_event.hpp>
#include <stratosphere/sf/sf_lmem_utility.hpp>
#include <stratosphere/usb/usb_limits.hpp>
#include <stratosphere/usb/usb_device_types.hpp>
#include <stratosphere/usb/ds/usb_i_ds_service.hpp>

namespace ams::usb {

    class DsInterface;
    class DsEndpoint;

    class DsClient {
        friend class DsInterface;
        friend class DsEndpoint;
        private:
            /* NOTE: Nintendo uses a UnitHeap here on newer firmware versions. */
            /* For now, we'll use an ExpHeap and do it the old way. */
            sf::ExpHeapAllocator m_allocator{};
            u8 m_heap_buffer[32_KB];
            lmem::HeapHandle m_heap_handle{};
            sf::SharedPointer<ds::IDsRootService> m_root_service{};
            sf::SharedPointer<ds::IDsService> m_ds_service{};
            bool m_is_initialized{false};
            std::atomic<int> m_reference_count{0};
            os::SystemEventType m_state_change_event{};
            DsInterface *m_interfaces[DsLimitMaxInterfacesPerConfigurationCount]{};
            bool m_is_enabled{false};
        public:
            DsClient() = default;
            ~DsClient() { /* ... */ }
        public:
            Result Initialize(ComplexId complex_id);
            Result Finalize();

            bool IsInitialized();

            Result EnableDevice();
            Result DisableDevice();

            os::SystemEventType *GetStateChangeEvent();
            Result GetState(UsbState *out);

            Result ClearDeviceData();

            Result AddUsbStringDescriptor(u8 *out_index, UsbStringDescriptor *desc);
            Result DeleteUsbStringDescriptor(u8 index);

            Result SetUsbDeviceDescriptor(UsbDeviceDescriptor *desc, UsbDeviceSpeed speed);

            Result SetBinaryObjectStore(u8 *data, int size);
        private:
            Result AddInterface(DsInterface *intf, sf::SharedPointer<ds::IDsInterface> *out_srv, uint8_t bInterfaceNumber);
            Result DeleteInterface(uint8_t bInterfaceNumber);
    };

    class DsInterface {
        friend class DsEndpoint;
        private:
            DsClient *m_client;
            sf::SharedPointer<ds::IDsInterface> m_interface;
            bool m_is_initialized;
            std::atomic<int> m_reference_count;
            os::SystemEventType m_setup_event;
            os::SystemEventType m_ctrl_in_completion_event;
            os::SystemEventType m_ctrl_out_completion_event;
            UrbReport m_report;
            u8 m_interface_num;
            DsEndpoint *m_endpoints[UsbLimitMaxEndpointsCount];
        public:
            DsInterface() : m_client(nullptr), m_is_initialized(false), m_reference_count(0) { /* ... */ }
            ~DsInterface() { /* ... */ }
        public:
            Result Initialize(DsClient *client, u8 bInterfaceNumber);
            Result Finalize();

            Result AppendConfigurationData(UsbDeviceSpeed speed, void *data, u32 size);

            bool IsInitialized();

            os::SystemEventType *GetSetupEvent();
            Result GetSetupPacket(UsbCtrlRequest *out);

            Result Enable();
            Result Disable();

            Result CtrlRead(u32 *out_transferred, void *dst, u32 size);
            Result CtrlWrite(u32 *out_transferred, void *dst, u32 size);
            Result CtrlDone();
            Result CtrlStall();
        private:
            Result AddEndpoint(DsEndpoint *ep, u8 bEndpointAddress, sf::SharedPointer<ds::IDsEndpoint> *out);
            Result DeleteEndpoint(u8 bEndpointAddress);

            Result CtrlIn(u32 *out_transferred, void *dst, u32 size);
            Result CtrlOut(u32 *out_transferred, void *dst, u32 size);
    };

    class DsEndpoint {
        private:
            bool m_is_initialized;
            bool m_is_new_format;
            std::atomic<int> m_reference_count;
            DsInterface *m_interface;
            sf::SharedPointer<ds::IDsEndpoint> m_endpoint;
            u8 m_address;
            os::SystemEventType m_completion_event;
            os::SystemEventType m_unknown_event;
        public:
            DsEndpoint() : m_is_initialized(false), m_is_new_format(false), m_reference_count(0) { /* ... */ }
        public:
            Result Initialize(DsInterface *interface, u8 bEndpointAddress);
            Result Finalize();

            bool IsInitialized();

            Result PostBuffer(u32 *out_transferred, void *buf, u32 size);
            Result PostBufferAsync(u32 *out_urb_id, void *buf, u32 size);

            os::SystemEventType *GetCompletionEvent();

            Result GetUrbReport(UrbReport *out);

            Result Cancel();

            Result SetZeroLengthTransfer(bool zlt);
    };

}
