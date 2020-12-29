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
#include "impl/sdmmc_sd_card_device_accessor.hpp"
#include "impl/sdmmc_port_mmc0.hpp"
#include "impl/sdmmc_port_sd_card0.hpp"
#include "impl/sdmmc_port_gc_asic0.hpp"

namespace ams::sdmmc {

    namespace {

        impl::SdCardDeviceAccessor *GetSdCardDeviceAccessor(Port port) {
            /* Get the accessor. */
            impl::SdCardDeviceAccessor *sd_card_device_accessor = nullptr;
            switch (port) {
                case Port_SdCard0: sd_card_device_accessor = impl::GetSdCardDeviceAccessorOfPortSdCard0(); break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            /* Ensure it's valid */
            AMS_ABORT_UNLESS(sd_card_device_accessor != nullptr);
            return sd_card_device_accessor;
        }

    }

    void SetSdCardWorkBuffer(Port port, void *buffer, size_t buffer_size) {
        return GetSdCardDeviceAccessor(port)->SetSdCardWorkBuffer(buffer, buffer_size);
    }

    void PutSdCardToSleep(Port port) {
        return GetSdCardDeviceAccessor(port)->PutSdCardToSleep();
    }

    void AwakenSdCard(Port port) {
        return GetSdCardDeviceAccessor(port)->AwakenSdCard();
    }

    Result GetSdCardProtectedAreaCapacity(u32 *out_num_sectors, Port port) {
        return GetSdCardDeviceAccessor(port)->GetSdCardProtectedAreaCapacity(out_num_sectors);
    }

    Result GetSdCardScr(void *dst, size_t dst_size, Port port) {
        return GetSdCardDeviceAccessor(port)->GetSdCardScr(dst, dst_size);
    }

    Result GetSdCardSwitchFunctionStatus(void *dst, size_t dst_size, Port port, SdCardSwitchFunction switch_function) {
        return GetSdCardDeviceAccessor(port)->GetSdCardSwitchFunctionStatus(dst, dst_size, switch_function);
    }

    Result GetSdCardCurrentConsumption(u16 *out_current_consumption, Port port, SpeedMode speed_mode) {
        return GetSdCardDeviceAccessor(port)->GetSdCardCurrentConsumption(out_current_consumption, speed_mode);
    }

    Result GetSdCardSdStatus(void *dst, size_t dst_size, Port port) {
        return GetSdCardDeviceAccessor(port)->GetSdCardSdStatus(dst, dst_size);
    }

    Result CheckSdCardConnection(SpeedMode *out_speed_mode, BusWidth *out_bus_width, Port port) {
        return GetSdCardDeviceAccessor(port)->CheckConnection(out_speed_mode, out_bus_width);
    }

    bool IsSdCardInserted(Port port) {
        return GetSdCardDeviceAccessor(port)->IsSdCardInserted();
    }

    bool IsSdCardRemoved(Port port) {
        return GetSdCardDeviceAccessor(port)->IsSdCardRemoved();
    }


    void RegisterSdCardDetectionEventCallback(Port port, DeviceDetectionEventCallback callback, void *arg) {
        return GetSdCardDeviceAccessor(port)->RegisterSdCardDetectionEventCallback(callback, arg);
    }

    void UnregisterSdCardDetectionEventCallback(Port port) {
        return GetSdCardDeviceAccessor(port)->UnregisterSdCardDetectionEventCallback();
    }

}

