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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/results.hpp>
#include <vapours/util.hpp>
#include <vapours/svc.hpp>
#include <vapours/dd.hpp>

#if defined(ATMOSPHERE_IS_EXOSPHERE)

    //#define AMS_SDMMC_THREAD_SAFE
    //#define AMS_SDMMC_USE_DEVICE_VIRTUAL_ADDRESS
    //#define AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL
    //#define AMS_SDMMC_USE_DEVICE_DETECTOR
    //#define AMS_SDMMC_USE_LOGGER
    //#define AMS_SDMMC_USE_OS_EVENTS
    //#define AMS_SDMMC_USE_OS_TIMER
    #define AMS_SDMMC_USE_UTIL_TIMER
    //#define AMS_SDMMC_ENABLE_MMC_HS400
    //#define AMS_SDMMC_ENABLE_SD_UHS_I
    //#define AMS_SDMMC_SET_PLLC4_BASE
    //#define AMS_SDMMC_USE_SD_CARD_DETECTOR

#elif defined(ATMOSPHERE_IS_MESOSPHERE)

    //#define AMS_SDMMC_THREAD_SAFE
    //#define AMS_SDMMC_USE_DEVICE_VIRTUAL_ADDRESS
    //#define AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL
    //#define AMS_SDMMC_USE_DEVICE_DETECTOR
    //#define AMS_SDMMC_USE_LOGGER
    //#define AMS_SDMMC_USE_OS_EVENTS
    //#define AMS_SDMMC_USE_OS_TIMER
    #define AMS_SDMMC_USE_UTIL_TIMER
    //#define AMS_SDMMC_ENABLE_MMC_HS400
    //#define AMS_SDMMC_ENABLE_SD_UHS_I
    //#define AMS_SDMMC_SET_PLLC4_BASE
    //#define AMS_SDMMC_USE_SD_CARD_DETECTOR

#elif defined(ATMOSPHERE_IS_STRATOSPHERE)

    #define AMS_SDMMC_THREAD_SAFE
    #define AMS_SDMMC_USE_DEVICE_VIRTUAL_ADDRESS
    #define AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL
    #define AMS_SDMMC_USE_DEVICE_DETECTOR
    #define AMS_SDMMC_USE_LOGGER
    #define AMS_SDMMC_USE_OS_EVENTS
    #define AMS_SDMMC_USE_OS_TIMER
    //#define AMS_SDMMC_USE_UTIL_TIMER
    #define AMS_SDMMC_ENABLE_MMC_HS400
    #define AMS_SDMMC_ENABLE_SD_UHS_I
    #define AMS_SDMMC_SET_PLLC4_BASE
    #define AMS_SDMMC_USE_SD_CARD_DETECTOR

#else
    #error "Unknown execution context for ams::sdmmc!"
#endif
