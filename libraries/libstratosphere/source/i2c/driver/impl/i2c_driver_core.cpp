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
#include "i2c_driver_core.hpp"

namespace ams::i2c::driver::impl {

    namespace {

        constinit os::SdkMutex g_init_mutex;
        constinit int g_init_count = 0;

        i2c::driver::II2cDriver::List &GetI2cDriverList() {
            static i2c::driver::II2cDriver::List s_driver_list;
            return s_driver_list;
        }

        ddsf::DeviceCodeEntryManager &GetDeviceCodeEntryManager() {
            static ddsf::DeviceCodeEntryManager s_device_code_entry_manager(ddsf::GetDeviceCodeEntryHolderMemoryResource());
            return s_device_code_entry_manager;
        }

    }


    void InitializeDrivers() {
        std::scoped_lock lk(g_init_mutex);

        /* Initialize all registered drivers, if this is our first initialization. */
        if ((g_init_count++) == 0) {
            for (auto &driver : GetI2cDriverList()) {
                driver.SafeCastTo<II2cDriver>().InitializeDriver();
            }
        }
    }

    void FinalizeDrivers() {
        std::scoped_lock lk(g_init_mutex);

        /* If we have no remaining sessions, close. */
        if ((--g_init_count) == 0) {
            /* Reset all device code entries. */
            GetDeviceCodeEntryManager().Reset();

            /* Finalize all drivers. */
            for (auto &driver : GetI2cDriverList()) {
                driver.SafeCastTo<II2cDriver>().FinalizeDriver();
            }
        }
    }

    void RegisterDriver(II2cDriver *driver) {
        AMS_ASSERT(driver != nullptr);
        GetI2cDriverList().push_back(*driver);
    }

    void UnregisterDriver(II2cDriver *driver) {
        AMS_ASSERT(driver != nullptr);
        if (driver->IsLinkedToList()) {
            auto &list = GetI2cDriverList();
            list.erase(list.iterator_to(*driver));
        }
    }

    Result RegisterDeviceCode(DeviceCode device_code, I2cDeviceProperty *device) {
        AMS_ASSERT(device != nullptr);
        R_TRY(GetDeviceCodeEntryManager().Add(device_code, device));
        return ResultSuccess();
    }

    bool UnregisterDeviceCode(DeviceCode device_code) {
        return GetDeviceCodeEntryManager().Remove(device_code);
    }

    Result FindDevice(I2cDeviceProperty **out, DeviceCode device_code) {
        /* Validate output. */
        AMS_ASSERT(out != nullptr);

        /* Find the device. */
        ddsf::IDevice *device;
        R_TRY(GetDeviceCodeEntryManager().FindDevice(std::addressof(device), device_code));

        /* Set output. */
        *out = device->SafeCastToPointer<I2cDeviceProperty>();
        return ResultSuccess();
    }

    Result FindDeviceByBusIndexAndAddress(I2cDeviceProperty **out, i2c::I2cBus bus_index, u16 slave_address) {
        /* Validate output. */
        AMS_ASSERT(out != nullptr);

        /* Convert the bus index to a device code. */
        const DeviceCode device_code = ConvertToDeviceCode(bus_index);

        /* Find the device. */
        bool found = false;
        GetDeviceCodeEntryManager().ForEachEntry([&](ddsf::DeviceCodeEntry &entry) -> bool {
            /* Convert the entry to an I2cDeviceProperty. */
            auto &device = entry.GetDevice().SafeCastTo<I2cDeviceProperty>();
            auto &driver = device.GetDriver().SafeCastTo<II2cDriver>();

            /* Check if the device is the one we're looking for. */
            if (driver.GetDeviceCode() == device_code && device.GetAddress() == slave_address) {
                found = true;
                *out = std::addressof(device);
                return false;
            }
            return true;
        });

        /* Check that we found the pad. */
        R_UNLESS(found, ddsf::ResultDeviceCodeNotFound());

        return ResultSuccess();
    }

}
