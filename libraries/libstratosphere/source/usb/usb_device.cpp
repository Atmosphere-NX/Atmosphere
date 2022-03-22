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
#include "usb_remote_ds_root_service.hpp"
#include "usb_remote_ds_service.hpp"
#include "impl/usb_util.hpp"

namespace ams::usb {

    Result DsClient::Initialize(ComplexId complex_id) {
        /* Clear interfaces. */
        for (size_t i = 0; i < util::size(m_interfaces); ++i) {
            m_interfaces[i] = nullptr;
        }

        /* Initialize heap. */
        m_heap_handle = lmem::CreateExpHeap(m_heap_buffer, sizeof(m_heap_buffer), lmem::CreateOption_None);
        R_UNLESS(m_heap_handle != nullptr, usb::ResultMemAllocFailure());

        /* Attach our allocator. */
        m_allocator.Attach(m_heap_handle);

        /* Connect to usb:ds. */
        /* NOTE: Here, Nintendo does m_domain.InitializeByDomain<...>(...); m_domain.SetSessionCount(1); */
        {
            Service srv;
            R_TRY(sm::GetService(std::addressof(srv), sm::ServiceName::Encode("usb:ds")));

            R_ABORT_UNLESS(serviceConvertToDomain(std::addressof(srv)));

            using Allocator     = decltype(m_allocator);
            using ObjectFactory = sf::ObjectFactory<Allocator::Policy>;

            if (hos::GetVersion() >= hos::Version_11_0_0) {
                m_root_service = ObjectFactory::CreateSharedEmplaced<ds::IDsRootService, RemoteDsRootService>(std::addressof(m_allocator), srv, std::addressof(m_allocator));

                R_TRY(m_root_service->GetService(std::addressof(m_ds_service)));
            } else {
                m_ds_service   = ObjectFactory::CreateSharedEmplaced<ds::IDsService, RemoteDsService>(std::addressof(m_allocator), srv, std::addressof(m_allocator));
            }
        }

        /* Bind the client process. */
        R_TRY(m_ds_service->Bind(complex_id, sf::CopyHandle(dd::GetCurrentProcessHandle(), false)));

        /* Get the state change event. */
        sf::NativeHandle event_handle;
        R_TRY(m_ds_service->GetStateChangeEvent(std::addressof(event_handle)));

        /* Attach the state change event handle to our event. */
        os::AttachReadableHandleToSystemEvent(std::addressof(m_state_change_event), event_handle.GetOsHandle(), event_handle.IsManaged(), os::EventClearMode_ManualClear);
        event_handle.Detach();

        /* Mark ourselves as initialized. */
        m_is_initialized = true;

        return ResultSuccess();
    }

    Result DsClient::Finalize() {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Disable and finalize all interfaces. */
        R_TRY(this->DisableDevice());
        for (size_t i = 0; i < util::size(m_interfaces); ++i) {
            if (m_interfaces[i] != nullptr) {
                R_TRY(m_interfaces[i]->Finalize());
            }
        }

        /* Check our reference count .*/
        R_UNLESS(m_reference_count <= 1, usb::ResultResourceBusy());

        /* Finalize members. */
        m_is_initialized = false;
        os::DestroySystemEvent(std::addressof(m_state_change_event));
        lmem::DestroyExpHeap(m_heap_handle);
        m_heap_handle = nullptr;

        /* Destroy interface objects. */
        m_ds_service   = nullptr;
        m_root_service = nullptr;

        return ResultSuccess();
    }

    bool DsClient::IsInitialized() {
        return m_is_initialized;
    }

    Result DsClient::EnableDevice() {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Enable all interfaces. */
        if (hos::GetVersion() < hos::Version_11_0_0) {
            for (size_t i = 0; i < util::size(m_interfaces); ++i) {
                if (m_interfaces[i] != nullptr) {
                    R_TRY(m_interfaces[i]->Enable());
                }
            }
        }

        /* Enable the device. */
        R_TRY(m_ds_service->Enable());

        /* Mark disabled. */
        m_is_enabled = true;
        return ResultSuccess();
    }

    Result DsClient::DisableDevice() {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Disable the device. */
        R_TRY(m_ds_service->Disable());

        /* Disable all interfaces. */
        if (hos::GetVersion() < hos::Version_11_0_0) {
            for (size_t i = 0; i < util::size(m_interfaces); ++i) {
                if (m_interfaces[i] != nullptr) {
                    R_TRY(m_interfaces[i]->Disable());
                }
            }
        }

        /* Mark disabled. */
        m_is_enabled = false;
        return ResultSuccess();
    }

    os::SystemEventType *DsClient::GetStateChangeEvent() {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        return m_is_initialized ? std::addressof(m_state_change_event) : nullptr;
    }

    Result DsClient::GetState(UsbState *out) {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Check that we have a service. */
        AMS_ABORT_UNLESS(m_ds_service != nullptr);

        return m_ds_service->GetState(out);
    }

    Result DsClient::ClearDeviceData() {
        return m_ds_service->ClearDeviceData();
    }

    Result DsClient::AddUsbStringDescriptor(u8 *out_index, UsbStringDescriptor *desc) {
        return m_ds_service->AddUsbStringDescriptor(out_index, sf::InBuffer(reinterpret_cast<const u8 *>(desc), sizeof(*desc)));
    }

    Result DsClient::DeleteUsbStringDescriptor(u8 index) {
        return m_ds_service->DeleteUsbStringDescriptor(index);
    }

    Result DsClient::SetUsbDeviceDescriptor(UsbDeviceDescriptor *desc, UsbDeviceSpeed speed) {
        return m_ds_service->SetUsbDeviceDescriptor(sf::InBuffer(reinterpret_cast<const u8 *>(desc), sizeof(*desc)), speed);
    }

    Result DsClient::SetBinaryObjectStore(u8 *data, int size) {
        return m_ds_service->SetBinaryObjectStore(sf::InBuffer(reinterpret_cast<const u8 *>(data), size));
    }

    Result DsClient::AddInterface(DsInterface *intf, sf::SharedPointer<ds::IDsInterface> *out_srv, uint8_t bInterfaceNumber) {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Check that we have a service. */
        AMS_ABORT_UNLESS(m_ds_service != nullptr);

        /* Register the interface. */
        R_TRY(m_ds_service->RegisterInterface(out_srv, bInterfaceNumber));

        /* Set interface. */
        m_interfaces[bInterfaceNumber] = intf;

        return ResultSuccess();
    }

    Result DsClient::DeleteInterface(uint8_t bInterfaceNumber) {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Check that we have the interface. */
        R_UNLESS(m_interfaces[bInterfaceNumber] != nullptr, usb::ResultOperationDenied());

        /* Clear the interface. */
        m_interfaces[bInterfaceNumber] = nullptr;

        return ResultSuccess();
    }

    Result DsInterface::Initialize(DsClient *client, u8 bInterfaceNumber) {
        /* Check that we haven't already initialized. */
        R_UNLESS(!m_is_initialized, usb::ResultAlreadyInitialized());

        /* Set our client. */
        m_client = client;

        /* Clear all endpoints. */
        for (size_t i = 0; i < util::size(m_endpoints); ++i) {
            m_endpoints[i] = nullptr;
        }

        /* Set our interface number. */
        R_UNLESS(bInterfaceNumber < util::size(m_client->m_interfaces), usb::ResultInvalidParameter());
        m_interface_num = bInterfaceNumber;

        /* Add the interface. */
        R_TRY(m_client->AddInterface(this, std::addressof(m_interface), m_interface_num));

        /* Ensure we cleanup if we fail after this. */
        auto intf_guard = SCOPE_GUARD { m_client->DeleteInterface(m_interface_num); m_interface = nullptr; };

        /* Get events. */
        sf::NativeHandle setup_event_handle;
        sf::NativeHandle ctrl_in_event_handle;
        sf::NativeHandle ctrl_out_event_handle;
        R_TRY(m_interface->GetSetupEvent(std::addressof(setup_event_handle)));
        R_TRY(m_interface->GetCtrlInCompletionEvent(std::addressof(ctrl_in_event_handle)));
        R_TRY(m_interface->GetCtrlOutCompletionEvent(std::addressof(ctrl_out_event_handle)));

        /* Attach events. */
        os::AttachReadableHandleToSystemEvent(std::addressof(m_setup_event), setup_event_handle.GetOsHandle(), setup_event_handle.IsManaged(), os::EventClearMode_ManualClear);
        os::AttachReadableHandleToSystemEvent(std::addressof(m_ctrl_in_completion_event), ctrl_in_event_handle.GetOsHandle(), ctrl_in_event_handle.IsManaged(), os::EventClearMode_ManualClear);
        os::AttachReadableHandleToSystemEvent(std::addressof(m_ctrl_out_completion_event), ctrl_out_event_handle.GetOsHandle(), ctrl_out_event_handle.IsManaged(), os::EventClearMode_ManualClear);

        setup_event_handle.Detach();
        ctrl_in_event_handle.Detach();
        ctrl_out_event_handle.Detach();

        /* Increment our client's reference count. */
        ++m_client->m_reference_count;

        /* Set ourselves as initialized. */
        m_is_initialized = true;

        intf_guard.Cancel();
        return ResultSuccess();
    }

    Result DsInterface::Finalize() {
        /* Validate that we have a service. */
        AMS_ABORT_UNLESS(m_interface != nullptr);

        /* We must be disabled. */
        R_UNLESS(!m_client->m_is_enabled, usb::ResultResourceBusy());

        /* Finalize all endpoints. */
        for (size_t i = 0; i < util::size(m_endpoints); ++i) {
            if (m_endpoints[i] != nullptr) {
                R_TRY(m_endpoints[i]->Finalize());
            }
        }

        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Check our reference count .*/
        R_UNLESS(m_reference_count <= 1, usb::ResultResourceBusy());

        /* Finalize members. */
        m_is_initialized = false;
        os::DestroySystemEvent(std::addressof(m_setup_event));
        os::DestroySystemEvent(std::addressof(m_ctrl_in_completion_event));
        os::DestroySystemEvent(std::addressof(m_ctrl_out_completion_event));

        /* Delete ourselves from our cleint. */
        m_client->DeleteInterface(m_interface_num);

        /* Destroy our service. */
        m_interface = nullptr;

        /* Close our reference to our client. */
        --m_client->m_reference_count;
        m_client = nullptr;

        return ResultSuccess();
    }

    Result DsInterface::AppendConfigurationData(UsbDeviceSpeed speed, void *data, u32 size) {
        return m_interface->AppendConfigurationData(m_interface_num, speed, sf::InBuffer(data, size));
    }

    bool DsInterface::IsInitialized() {
        return m_is_initialized;
    }

    os::SystemEventType *DsInterface::GetSetupEvent() {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        return m_is_initialized ? std::addressof(m_setup_event) : nullptr;
    }

    Result DsInterface::GetSetupPacket(UsbCtrlRequest *out) {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Check that we have a service. */
        AMS_ABORT_UNLESS(m_interface != nullptr);

        return m_interface->GetSetupPacket(sf::OutBuffer(out, sizeof(*out)));
    }

    Result DsInterface::Enable() {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* If we're already enabled, nothing to do. */
        R_SUCCEED_IF(m_client->m_is_enabled);

        /* Check that we have a service. */
        AMS_ABORT_UNLESS(m_interface != nullptr);

        /* Perform the enable. */
        R_TRY(m_interface->Enable());

        return ResultSuccess();
    }

    Result DsInterface::Disable() {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* If we're already disabled, nothing to do. */
        R_SUCCEED_IF(!m_client->m_is_enabled);

        /* Check that we have a service. */
        AMS_ABORT_UNLESS(m_interface != nullptr);

        /* Perform the disable. */
        R_TRY(m_interface->Disable());

        return ResultSuccess();
    }

    Result DsInterface::AddEndpoint(DsEndpoint *ep, u8 bEndpointAddress, sf::SharedPointer<ds::IDsEndpoint> *out) {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Check that we're not already enabled. */
        R_UNLESS(!m_client->m_is_enabled, usb::ResultOperationDenied());

        /* Check that we have a service. */
        AMS_ABORT_UNLESS(m_interface != nullptr);

        /* Register the endpoint. */
        R_TRY(m_interface->RegisterEndpoint(bEndpointAddress, out));

        /* Set the endpoint. */
        m_endpoints[impl::GetEndpointIndex(bEndpointAddress)] = ep;

        return ResultSuccess();
    }

    Result DsInterface::DeleteEndpoint(u8 bEndpointAddress) {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Check that we're disabled and have the endpoint. */
        const auto index = impl::GetEndpointIndex(bEndpointAddress);
        R_UNLESS(!m_client->m_is_enabled,       usb::ResultOperationDenied());
        R_UNLESS(m_endpoints[index] != nullptr, usb::ResultOperationDenied());

        /* Clear the endpoint. */
        m_endpoints[index] = nullptr;

        return ResultSuccess();
    }

    Result DsInterface::CtrlIn(u32 *out_transferred, void *dst, u32 size) {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Check that we're enabled. */
        R_UNLESS(m_client->m_is_enabled, usb::ResultOperationDenied());

        /* Check that the data is aligned. */
        R_UNLESS(usb::IsDmaAligned(reinterpret_cast<u64>(dst)), usb::ResultAlignmentError());

        /* If we should, flush cache. */
        if (size != 0) {
            dd::FlushDataCache(dst, size);
        }

        /* Check that we have a service. */
        AMS_ABORT_UNLESS(m_interface != nullptr);

        /* Perform the transfer. */
        u32 urb_id;
        R_TRY(m_interface->CtrlInAsync(std::addressof(urb_id), reinterpret_cast<u64>(dst), size));

        /* Wait for control to finish. */
        os::WaitSystemEvent(std::addressof(m_ctrl_in_completion_event));
        os::ClearSystemEvent(std::addressof(m_ctrl_in_completion_event));

        /* Get the urb report. */
        R_ABORT_UNLESS(m_interface->GetCtrlInUrbReport(std::addressof(m_report)));

        /* Check the report is for our urb. */
        R_UNLESS(m_report.count == 1,              usb::ResultInternalStateError());
        R_UNLESS(m_report.reports[0].id == urb_id, usb::ResultInternalStateError());

        /* Set output bytes. */
        if (out_transferred != nullptr) {
            *out_transferred = m_report.reports[0].transferred_size;
        }

        /* Handle the report. */
        switch (m_report.reports[0].status) {
            case UrbStatus_Cancelled:
                return usb::ResultInterrupted();
            case UrbStatus_Failed:
                return usb::ResultTransactionError();
            case UrbStatus_Finished:
                return ResultSuccess();
            default:
                return usb::ResultInternalStateError();
        }
    }

    Result DsInterface::CtrlOut(u32 *out_transferred, void *dst, u32 size) {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Check that we're enabled. */
        R_UNLESS(m_client->m_is_enabled, usb::ResultOperationDenied());

        /* Check that the data is aligned. */
        R_UNLESS(usb::IsDmaAligned(reinterpret_cast<u64>(dst)), usb::ResultAlignmentError());

        /* If we should, invalidate cache. */
        if (size != 0) {
            dd::InvalidateDataCache(dst, size);
        }

        /* Check that we have a service. */
        AMS_ABORT_UNLESS(m_interface != nullptr);

        /* Perform the transfer. */
        u32 urb_id;
        R_TRY(m_interface->CtrlOutAsync(std::addressof(urb_id), reinterpret_cast<u64>(dst), size));

        /* Wait for control to finish. */
        os::WaitSystemEvent(std::addressof(m_ctrl_out_completion_event));
        os::ClearSystemEvent(std::addressof(m_ctrl_out_completion_event));

        /* Ensure that cache remains consistent. */
        ON_SCOPE_EXIT {
            if (size != 0) {
                dd::InvalidateDataCache(dst, size);
            }
        };

        /* Get the urb report. */
        R_ABORT_UNLESS(m_interface->GetCtrlOutUrbReport(std::addressof(m_report)));

        /* Check the report is for our urb. */
        R_UNLESS(m_report.count == 1,              usb::ResultInternalStateError());
        R_UNLESS(m_report.reports[0].id == urb_id, usb::ResultInternalStateError());

        /* Set output bytes. */
        if (out_transferred != nullptr) {
            *out_transferred = m_report.reports[0].transferred_size;
        }

        /* Handle the report. */
        switch (m_report.reports[0].status) {
            case UrbStatus_Cancelled:
                return usb::ResultInterrupted();
            case UrbStatus_Failed:
                return usb::ResultTransactionError();
            case UrbStatus_Finished:
                return ResultSuccess();
            default:
                return usb::ResultInternalStateError();
        }
    }

    Result DsInterface::CtrlRead(u32 *out_transferred, void *dst, u32 size) {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Do the data transfer. */
        Result result = this->CtrlOut(out_transferred, dst, size);

        /* Do the status transfer. */
        if (R_SUCCEEDED(result)) {
            result = this->CtrlIn(nullptr, nullptr, 0);
        }

        /* If we failed, stall. */
        if (R_FAILED(result)) {
            result = this->CtrlStall();
        }

        return result;
    }

    Result DsInterface::CtrlWrite(u32 *out_transferred, void *dst, u32 size) {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Do the data transfer. */
        Result result = this->CtrlIn(out_transferred, dst, size);

        /* Do the status transfer. */
        if (R_SUCCEEDED(result)) {
            result = this->CtrlOut(nullptr, nullptr, 0);
        }

        /* If we failed, stall. */
        if (R_FAILED(result)) {
            result = this->CtrlStall();
        }

        return result;
    }

    Result DsInterface::CtrlDone() {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Do the status transfer. */
        Result result = this->CtrlIn(nullptr, nullptr, 0);

        /* If we failed, stall. */
        if (R_FAILED(result)) {
            result = this->CtrlStall();
        }

        return result;
    }

    Result DsInterface::CtrlStall() {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Check that we're enabled. */
        R_UNLESS(m_client->m_is_enabled, usb::ResultOperationDenied());

        /* Check that we have a service. */
        AMS_ABORT_UNLESS(m_interface != nullptr);

        return m_interface->CtrlStall();
    }

    Result DsEndpoint::Initialize(DsInterface *interface, u8 bEndpointAddress) {
        /* Check that the interface is valid. */
        AMS_ABORT_UNLESS(interface != nullptr);

        /* Check that we're not already initialized. */
        R_UNLESS(!m_is_initialized, usb::ResultAlreadyInitialized());

        /* Set our interface. */
        m_interface = interface;

        /* Add the endpoint. */
        R_TRY(m_interface->AddEndpoint(this, bEndpointAddress, std::addressof(m_endpoint)));

        /* Set our address. */
        m_address = bEndpointAddress;

        /* Ensure we clean up if we fail after this. */
        auto ep_guard = SCOPE_GUARD { m_interface->DeleteEndpoint(m_address); m_endpoint = nullptr; };

        /* Get completion event. */
        sf::NativeHandle event_handle;
        R_TRY(m_endpoint->GetCompletionEvent(std::addressof(event_handle)));

        /* Increment our interface's reference count. */
        ++m_interface->m_reference_count;
        ++m_interface->m_client->m_reference_count;

        /* Attach our event. */
        os::AttachReadableHandleToSystemEvent(std::addressof(m_completion_event), event_handle.GetOsHandle(), event_handle.IsManaged(), os::EventClearMode_ManualClear);
        event_handle.Detach();

        /* Mark initialized. */
        m_is_initialized = true;

        ep_guard.Cancel();
        return ResultSuccess();
    }

    Result DsEndpoint::Finalize() {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Cancel any pending transactions. */
        m_endpoint->Cancel();

        /* Wait for us to be at one reference count. */
        while (m_reference_count > 1) {
            os::SleepThread(TimeSpan::FromMilliSeconds(25));
        }

        /* Destroy our event. */
        os::DestroySystemEvent(std::addressof(m_completion_event));

        /* Decrement our interface's reference count. */
        --m_interface->m_reference_count;
        --m_interface->m_client->m_reference_count;

        /* Delete ourselves. */
        R_TRY(m_interface->DeleteEndpoint(m_address));

        /* Clear ourselves. */
        m_interface = nullptr;
        m_endpoint  = nullptr;

        /* Mark uninitialized. */
        m_is_initialized = false;

        return ResultSuccess();
    }

    bool DsEndpoint::IsInitialized() {
        return m_is_initialized;
    }

    Result DsEndpoint::PostBuffer(u32 *out_transferred, void *buf, u32 size) {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Post buffer. */
        u32 urb_id;
        R_TRY(this->PostBufferAsync(std::addressof(urb_id), buf, size));

        /* Wait for completion. */
        os::WaitSystemEvent(std::addressof(m_completion_event));
        os::ClearSystemEvent(std::addressof(m_completion_event));

        /* Get URB report. */
        UrbReport report;
        AMS_ABORT_UNLESS(m_endpoint != nullptr);
        R_ABORT_UNLESS(m_endpoint->GetUrbReport(std::addressof(report)));

        /* Check the report is for our urb. */
        R_UNLESS(report.count == 1,              usb::ResultInternalStateError());
        R_UNLESS(report.reports[0].id == urb_id, usb::ResultInternalStateError());

        /* Set output bytes. */
        if (out_transferred != nullptr) {
            *out_transferred = report.reports[0].transferred_size;
        }

        /* Handle the report. */
        switch (report.reports[0].status) {
            case UrbStatus_Cancelled:
                return usb::ResultInterrupted();
            case UrbStatus_Failed:
                return usb::ResultTransactionError();
            case UrbStatus_Finished:
                return ResultSuccess();
            default:
                return usb::ResultInternalStateError();
        }
    }

    Result DsEndpoint::PostBufferAsync(u32 *out_urb_id, void *buf, u32 size) {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Check that the buffer is DMA aligned. */
        R_UNLESS(usb::IsDmaAligned(reinterpret_cast<u64>(buf)), usb::ResultAlignmentError());

        /* Check that we have a service. */
        AMS_ABORT_UNLESS(m_endpoint != nullptr);

        /* Post */
        u32 urb_id = 0;
        R_TRY(m_endpoint->PostBufferAsync(std::addressof(urb_id), reinterpret_cast<u64>(buf), size));

        *out_urb_id = urb_id;
        return ResultSuccess();
    }

    os::SystemEventType *DsEndpoint::GetCompletionEvent() {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        return m_is_initialized ? std::addressof(m_completion_event) : nullptr;
    }

    Result DsEndpoint::GetUrbReport(UrbReport *out) {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Check that we have a service. */
        AMS_ABORT_UNLESS(m_endpoint != nullptr);

        return m_endpoint->GetUrbReport(out);
    }

    Result DsEndpoint::Cancel() {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Check that we have a service. */
        AMS_ABORT_UNLESS(m_endpoint != nullptr);

        return m_endpoint->Cancel();
    }

    Result DsEndpoint::SetZeroLengthTransfer(bool zlt) {
        /* Create a scoped reference. */
        impl::ScopedRefCount scoped_ref(m_reference_count);

        /* Check that we're initialized. */
        R_UNLESS(m_is_initialized, usb::ResultNotInitialized());

        /* Check that we have a service. */
        AMS_ABORT_UNLESS(m_endpoint != nullptr);

        return m_endpoint->SetZlt(zlt);
    }

}
