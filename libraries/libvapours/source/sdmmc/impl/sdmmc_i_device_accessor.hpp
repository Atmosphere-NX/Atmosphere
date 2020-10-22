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
#pragma once
#include <vapours.hpp>
#include "sdmmc_i_host_controller.hpp"

namespace ams::sdmmc::impl {

    class IDeviceAccessor {
        public:
            virtual void Initialize() = 0;
            virtual void Finalize()   = 0;

            #if defined(AMS_SDMMC_USE_DEVICE_VIRTUAL_ADDRESS)
            virtual void RegisterDeviceVirtualAddress(uintptr_t buffer, size_t buffer_size, ams::dd::DeviceVirtualAddress buffer_device_virtual_address) = 0;
            virtual void UnregisterDeviceVirtualAddress(uintptr_t buffer, size_t buffer_size, ams::dd::DeviceVirtualAddress buffer_device_virtual_address) = 0;
            #endif

            virtual Result Activate() = 0;
            virtual void Deactivate() = 0;

            virtual Result ReadWrite(u32 sector_index, u32 num_sectors, void *buffer, size_t buffer_size, bool is_read) = 0;
            virtual Result CheckConnection(SpeedMode *out_speed_mode, BusWidth *out_bus_width) = 0;

            virtual Result GetSpeedMode(SpeedMode *out) const = 0;
            virtual Result GetMemoryCapacity(u32 *out_sectors) const = 0;
            virtual Result GetDeviceStatus(u32 *out) const = 0;
            virtual Result GetOcr(u32 *out) const = 0;
            virtual Result GetRca(u16 *out) const = 0;
            virtual Result GetCid(void *out, size_t size) const = 0;
            virtual Result GetCsd(void *out, size_t size) const = 0;

            virtual void GetAndClearErrorInfo(ErrorInfo *out_error_info, size_t *out_log_size, char *out_log_buffer, size_t log_buffer_size) = 0;
    };

}
