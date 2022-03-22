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
#include <stratosphere/dd/dd_device_address_space_common.hpp>

namespace ams::usb {

    constexpr inline int HwLimitDmaBufferAlignmentSize = dd::DeviceAddressSpaceMemoryRegionAlignment;
    constexpr inline int HwLimitDataCacheLineSize      = 0x40;
    constexpr inline int HwLimitMaxPortCount           = 0x4;

    constexpr inline int UsbLimitMaxEndpointsCount    = 0x20;
    constexpr inline int UsbLimitMaxEndpointPairCount = 0x10;

    constexpr inline int DsLimitMaxConfigurationsPerDeviceCount    = 1;
    constexpr inline int DsLimitMaxInterfacesPerConfigurationCount = 4;
    constexpr inline int DsLimitMaxNameSize                        = 0x40;
    constexpr inline int DsLimitRingSize                           = 8;

}
