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
#include "impl/sdmmc_gc_asic_device_accessor.hpp"
#include "impl/sdmmc_port_mmc0.hpp"
#include "impl/sdmmc_port_sd_card0.hpp"
#include "impl/sdmmc_port_gc_asic0.hpp"

namespace ams::sdmmc {

    namespace {

        impl::GcAsicDeviceAccessor *GetGcAsicDeviceAccessor(Port port) {
            /* Get the accessor. */
            impl::GcAsicDeviceAccessor *gc_asic_device_accessor = nullptr;
            switch (port) {
                case Port_GcAsic0: gc_asic_device_accessor = impl::GetGcAsicDeviceAccessorOfPortGcAsic0(); break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            /* Ensure it's valid */
            AMS_ABORT_UNLESS(gc_asic_device_accessor != nullptr);
            return gc_asic_device_accessor;
        }

    }

    void PutGcAsicToSleep(Port port) {
        return GetGcAsicDeviceAccessor(port)->PutGcAsicToSleep();
    }

    Result AwakenGcAsic(Port port) {
        return GetGcAsicDeviceAccessor(port)->AwakenGcAsic();
    }

    Result WriteGcAsicOperation(Port port, const void *op_buf, size_t op_buf_size) {
        return GetGcAsicDeviceAccessor(port)->WriteGcAsicOperation(op_buf, op_buf_size);
    }

    Result FinishGcAsicOperation(Port port) {
        return GetGcAsicDeviceAccessor(port)->FinishGcAsicOperation();
    }

    Result AbortGcAsicOperation(Port port) {
        return GetGcAsicDeviceAccessor(port)->AbortGcAsicOperation();
    }

    Result SleepGcAsic(Port port) {
        return GetGcAsicDeviceAccessor(port)->SleepGcAsic();
    }

    Result UpdateGcAsicKey(Port port) {
        return GetGcAsicDeviceAccessor(port)->UpdateGcAsicKey();
    }

    void SignalGcRemovedEvent(Port port) {
        return GetGcAsicDeviceAccessor(port)->SignalGcRemovedEvent();
    }

    void ClearGcRemovedEvent(Port port) {
        return GetGcAsicDeviceAccessor(port)->ClearGcRemovedEvent();
    }

}
