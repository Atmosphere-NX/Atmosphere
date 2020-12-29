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
#include "sdmmc_port_sd_card0.hpp"
#include "sdmmc_select_sdmmc_controller.hpp"


namespace ams::sdmmc::impl {

    namespace {

        SdmmcControllerForPortSdCard0 g_sd_card0_host_controller;

        #if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)

            constexpr inline u32 SdCard0DebounceMilliSeconds = 128;
            DeviceDetector g_sd_card0_detector(gpio::DeviceCode_SdCd, gpio::GpioValue_Low, SdCard0DebounceMilliSeconds);

            SdCardDeviceAccessor g_sd_card0_device_accessor(std::addressof(g_sd_card0_host_controller), std::addressof(g_sd_card0_detector));

        #else

            SdCardDeviceAccessor g_sd_card0_device_accessor(std::addressof(g_sd_card0_host_controller));

        #endif


    }

    IHostController *GetHostControllerOfPortSdCard0() {
        return std::addressof(g_sd_card0_host_controller);
    }

    IDeviceAccessor *GetDeviceAccessorOfPortSdCard0() {
        return std::addressof(g_sd_card0_device_accessor);
    }

    SdCardDeviceAccessor *GetSdCardDeviceAccessorOfPortSdCard0() {
        return std::addressof(g_sd_card0_device_accessor);
    }

}
