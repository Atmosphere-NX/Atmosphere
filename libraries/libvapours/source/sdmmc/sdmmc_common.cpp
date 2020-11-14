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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif
#include "impl/sdmmc_i_host_controller.hpp"
#include "impl/sdmmc_i_device_accessor.hpp"
#include "impl/sdmmc_clock_reset_controller.hpp"
#include "impl/sdmmc_port_mmc0.hpp"
#include "impl/sdmmc_port_sd_card0.hpp"
#include "impl/sdmmc_port_gc_asic0.hpp"

namespace ams::sdmmc {

    namespace {

        impl::IHostController *GetHostController(Port port) {
            /* Get the controller. */
            impl::IHostController *host_controller = nullptr;
            switch (port) {
                case Port_Mmc0:    host_controller = impl::GetHostControllerOfPortMmc0();    break;
                case Port_SdCard0: host_controller = impl::GetHostControllerOfPortSdCard0(); break;
                case Port_GcAsic0: host_controller = impl::GetHostControllerOfPortGcAsic0(); break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            /* Ensure it's valid */
            AMS_ABORT_UNLESS(host_controller != nullptr);
            return host_controller;
        }

        impl::IDeviceAccessor *GetDeviceAccessor(Port port) {
            /* Get the accessor. */
            impl::IDeviceAccessor *device_accessor = nullptr;
            switch (port) {
                case Port_Mmc0:    device_accessor = impl::GetDeviceAccessorOfPortMmc0();    break;
                case Port_SdCard0: device_accessor = impl::GetDeviceAccessorOfPortSdCard0(); break;
                case Port_GcAsic0: device_accessor = impl::GetDeviceAccessorOfPortGcAsic0(); break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            /* Ensure it's valid */
            AMS_ABORT_UNLESS(device_accessor != nullptr);
            return device_accessor;
        }

    }



    void Initialize(Port port) {
        return GetDeviceAccessor(port)->Initialize();
    }

    void Finalize(Port port) {
        return GetDeviceAccessor(port)->Finalize();
    }

#if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
    void SwitchToPcvClockResetControl() {
        return impl::ClockResetController::SwitchToPcvControl();
    }
#endif

#if defined(AMS_SDMMC_USE_DEVICE_VIRTUAL_ADDRESS)
    void RegisterDeviceVirtualAddress(Port port, uintptr_t buffer, size_t buffer_size, ams::dd::DeviceVirtualAddress buffer_device_virtual_address) {
        return GetDeviceAccessor(port)->RegisterDeviceVirtualAddress(buffer, buffer_size, buffer_device_virtual_address);
    }

    void UnregisterDeviceVirtualAddress(Port port, uintptr_t buffer, size_t buffer_size, ams::dd::DeviceVirtualAddress buffer_device_virtual_address) {
        return GetDeviceAccessor(port)->UnregisterDeviceVirtualAddress(buffer, buffer_size, buffer_device_virtual_address);
    }
#endif

    void ChangeCheckTransferInterval(Port port, u32 ms) {
        return GetHostController(port)->ChangeCheckTransferInterval(ms);
    }
    void SetDefaultCheckTransferInterval(Port port) {
        return GetHostController(port)->SetDefaultCheckTransferInterval();
    }

    Result Activate(Port port) {
        return GetDeviceAccessor(port)->Activate();
    }

    void Deactivate(Port port) {
        return GetDeviceAccessor(port)->Deactivate();
    }

    Result Read(void *dst, size_t dst_size, Port port, u32 sector_index, u32 num_sectors) {
        return GetDeviceAccessor(port)->ReadWrite(sector_index, num_sectors, dst, dst_size, true);
    }

    Result Write(Port port, u32 sector_index, u32 num_sectors, const void *src, size_t src_size) {
        return GetDeviceAccessor(port)->ReadWrite(sector_index, num_sectors, const_cast<void *>(src), src_size, false);
    }

    Result CheckConnection(SpeedMode *out_speed_mode, BusWidth *out_bus_width, Port port) {
        return GetDeviceAccessor(port)->CheckConnection(out_speed_mode, out_bus_width);
    }

    Result GetDeviceSpeedMode(SpeedMode *out, Port port) {
        return GetDeviceAccessor(port)->GetSpeedMode(out);
    }

    Result GetDeviceMemoryCapacity(u32 *out_num_sectors, Port port) {
        return GetDeviceAccessor(port)->GetMemoryCapacity(out_num_sectors);
    }

    Result GetDeviceStatus(u32 *out_device_status, Port port) {
        return GetDeviceAccessor(port)->GetDeviceStatus(out_device_status);
    }

    Result GetDeviceCid(void *out, size_t out_size, Port port) {
        return GetDeviceAccessor(port)->GetCid(out, out_size);
    }

    Result GetDeviceCsd(void *out, size_t out_size, Port port) {
        return GetDeviceAccessor(port)->GetCsd(out, out_size);
    }

    void GetAndClearErrorInfo(ErrorInfo *out_error_info, size_t *out_log_size, char *out_log_buffer, size_t log_buffer_size, Port port) {
        return GetDeviceAccessor(port)->GetAndClearErrorInfo(out_error_info, out_log_size, out_log_buffer, log_buffer_size);
    }

}
