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
#include "htclow_usb_driver.hpp"
#include "htclow_usb_impl.hpp"

namespace ams::htclow::driver {

    void UsbDriver::OnUsbAvailabilityChange(UsbAvailability availability, void *param) {
        /* Convert the argument to a driver. */
        UsbDriver *driver = static_cast<UsbDriver *>(param);

        /* Handle the change. */
        switch (availability) {
            case UsbAvailability_Unavailable:
                CancelUsbSendReceive();
                break;
            case UsbAvailability_Available:
                driver->m_event.Signal();
                break;
            case UsbAvailability_Unknown:
                driver->CancelSendReceive();
                break;
        }
    }

    Result UsbDriver::Open() {
        /* Clear our event. */
        m_event.Clear();

        /* Set the availability change callback. */
        SetUsbAvailabilityChangeCallback(OnUsbAvailabilityChange, this);

        /* Initialize the interface. */
        return InitializeUsbInterface();
    }

    void UsbDriver::Close() {
        /* Finalize the interface. */
        FinalizeUsbInterface();

        /* Clear the availability callback. */
        ClearUsbAvailabilityChangeCallback();
    }

    Result UsbDriver::Connect(os::EventType *event) {
        /* We must not already be connected. */
        AMS_ABORT_UNLESS(!m_connected);

        /* Perform a wait on our event. */
        const int idx = os::WaitAny(m_event.GetBase(), event);
        R_UNLESS(idx == 0, htclow::ResultCancelled());

        /* Clear our event. */
        m_event.Clear();

        /* We're connected. */
        m_connected = true;
        return ResultSuccess();
    }

    void UsbDriver::Shutdown() {
        /* If we're connected, cancel anything we're doing. */
        if (m_connected) {
            this->CancelSendReceive();
            m_connected = false;
        }
    }

    Result UsbDriver::Send(const void *src, int src_size) {
        /* Check size. */
        R_UNLESS(src_size >= 0, htclow::ResultInvalidArgument());

        /* Send until we've sent everything. */
        for (auto transferred = 0; transferred < src_size; /* ... */) {
            int cur;
            R_TRY(SendUsb(std::addressof(cur), reinterpret_cast<const void *>(reinterpret_cast<uintptr_t>(src) + transferred), src_size - transferred));

            transferred += cur;
        }

        return ResultSuccess();
    }

    Result UsbDriver::Receive(void *dst, int dst_size) {
        /* Check size. */
        R_UNLESS(dst_size >= 0, htclow::ResultInvalidArgument());

        /* Send until we've sent everything. */
        for (auto transferred = 0; transferred < dst_size; /* ... */) {
            int cur;
            R_TRY(ReceiveUsb(std::addressof(cur), reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(dst) + transferred), dst_size - transferred));

            transferred += cur;
        }

        return ResultSuccess();
    }

    void UsbDriver::CancelSendReceive() {
        CancelUsbSendReceive();
    }

    void UsbDriver::Suspend() {
        this->Close();
    }

    void UsbDriver::Resume() {
        R_ABORT_UNLESS(this->Open());
    }

}
