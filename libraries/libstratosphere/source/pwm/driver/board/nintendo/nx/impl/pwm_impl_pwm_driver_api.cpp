/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "pwm_impl_pwm_driver_api.hpp"
#include "pwm_pwm_driver_impl.hpp"

namespace ams::pwm::driver::board::nintendo::nx::impl {

    namespace {

        constexpr inline const dd::PhysicalAddress PwmRegistersPhysicalAddress = 0x7000A000;
        constexpr inline size_t PwmRegistersSize = 0x100;

        constexpr const ChannelDefinition SupportedChannels[] = {
            { pwm::DeviceCode_LcdBacklight, 0 },
            { pwm::DeviceCode_CpuFan,       1 },
        };

    }

    Result InitializePwmDriver() {
        /* Get the memory resource with which to allocate our driver/devices. */
        auto *memory_resource = ddsf::GetMemoryResource();

        /* Allocate storage for our driver. */
        auto *driver_storage = memory_resource->Allocate(sizeof(PwmDriverImpl), alignof(PwmDriverImpl));
        AMS_ABORT_UNLESS(driver_storage != nullptr);

        /* Create our driver. */
        auto *driver = new (static_cast<PwmDriverImpl *>(driver_storage)) PwmDriverImpl(PwmRegistersPhysicalAddress, PwmRegistersSize, SupportedChannels, util::size(SupportedChannels));

        /* Register our driver. */
        pwm::driver::RegisterDriver(driver);

        /* Create our devices. */
        for (const auto &entry : SupportedChannels) {
            auto *device_storage = memory_resource->Allocate(sizeof(PwmDriverImpl), alignof(PwmDriverImpl));
            AMS_ABORT_UNLESS(device_storage != nullptr);

            /* Create our driver. */
            auto *device = new (static_cast<PwmDeviceImpl *>(device_storage)) PwmDeviceImpl(entry.channel_id);

            /* Register the device with our driver. */
            driver->RegisterDevice(device);

            /* Register the device code with our driver. */
            R_ABORT_UNLESS(pwm::driver::RegisterDeviceCode(entry.device_code, device));
        }

        return ResultSuccess();
    }

}
