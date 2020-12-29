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
#include "impl/sdmmc_mmc_device_accessor.hpp"
#include "impl/sdmmc_port_mmc0.hpp"
#include "impl/sdmmc_port_sd_card0.hpp"
#include "impl/sdmmc_port_gc_asic0.hpp"

namespace ams::sdmmc {

    namespace {

        impl::MmcDeviceAccessor *GetMmcDeviceAccessor(Port port) {
            /* Get the accessor. */
            impl::MmcDeviceAccessor *mmc_device_accessor = nullptr;
            switch (port) {
                case Port_Mmc0: mmc_device_accessor = impl::GetMmcDeviceAccessorOfPortMmc0();    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            /* Ensure it's valid */
            AMS_ABORT_UNLESS(mmc_device_accessor != nullptr);
            return mmc_device_accessor;
        }

    }

    void SetMmcWorkBuffer(Port port, void *buffer, size_t buffer_size) {
        return GetMmcDeviceAccessor(port)->SetMmcWorkBuffer(buffer, buffer_size);
    }

    void PutMmcToSleep(Port port) {
        return GetMmcDeviceAccessor(port)->PutMmcToSleep();
    }

    void AwakenMmc(Port port) {
        return GetMmcDeviceAccessor(port)->AwakenMmc();
    }

    Result SelectMmcPartition(Port port, MmcPartition mmc_partition) {
        return GetMmcDeviceAccessor(port)->SelectMmcPartition(mmc_partition);
    }

    Result EraseMmc(Port port) {
        return GetMmcDeviceAccessor(port)->EraseMmc();
    }

    Result GetMmcBootPartitionCapacity(u32 *out_num_sectors, Port port) {
        return GetMmcDeviceAccessor(port)->GetMmcBootPartitionCapacity(out_num_sectors);
    }

    Result GetMmcExtendedCsd(void *out_buffer, size_t buffer_size, Port port) {
        return GetMmcDeviceAccessor(port)->GetMmcExtendedCsd(out_buffer, buffer_size);
    }

    Result CheckMmcConnection(SpeedMode *out_speed_mode, BusWidth *out_bus_width, Port port) {
        return GetMmcDeviceAccessor(port)->CheckConnection(out_speed_mode, out_bus_width);
    }

}
