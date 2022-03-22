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
#include <stratosphere.hpp>

namespace ams::htclow::driver {

    enum UsbAvailability {
        UsbAvailability_Unavailable = 1,
        UsbAvailability_Available   = 2,
        UsbAvailability_Unknown     = 3,
    };

    using UsbAvailabilityChangeCallback = void (*)(UsbAvailability availability, void *param);

    void SetUsbAvailabilityChangeCallback(UsbAvailabilityChangeCallback callback, void *param);
    void ClearUsbAvailabilityChangeCallback();

    Result InitializeUsbInterface();
    void FinalizeUsbInterface();

    Result SendUsb(int *out_transferred, const void *src, int src_size);
    Result ReceiveUsb(int *out_transferred, void *dst, int dst_size);

    void CancelUsbSendReceive();


}
