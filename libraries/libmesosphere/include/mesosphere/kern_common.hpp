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

namespace ams::kern {

    constexpr size_t PageSize = 4_KB;

    enum TargetFirmware : u32 {
        TargetFirmware_1_0_0  = ATMOSPHERE_TARGET_FIRMWARE_100,
        TargetFirmware_2_0_0  = ATMOSPHERE_TARGET_FIRMWARE_200,
        TargetFirmware_3_0_0  = ATMOSPHERE_TARGET_FIRMWARE_300,
        TargetFirmware_4_0_0  = ATMOSPHERE_TARGET_FIRMWARE_400,
        TargetFirmware_5_0_0  = ATMOSPHERE_TARGET_FIRMWARE_500,
        TargetFirmware_6_0_0  = ATMOSPHERE_TARGET_FIRMWARE_600,
        TargetFirmware_6_2_0  = ATMOSPHERE_TARGET_FIRMWARE_620,
        TargetFirmware_7_0_0  = ATMOSPHERE_TARGET_FIRMWARE_700,
        TargetFirmware_8_0_0  = ATMOSPHERE_TARGET_FIRMWARE_800,
        TargetFirmware_8_1_0  = ATMOSPHERE_TARGET_FIRMWARE_810,
        TargetFirmware_9_0_0  = ATMOSPHERE_TARGET_FIRMWARE_900,
        TargetFirmware_9_1_0  = ATMOSPHERE_TARGET_FIRMWARE_910,
        TargetFirmware_10_0_0 = ATMOSPHERE_TARGET_FIRMWARE_1000,
    };

    TargetFirmware GetTargetFirmware();

}

#if 1 || defined(AMS_BUILD_FOR_AUDITING)
#define MESOSPHERE_BUILD_FOR_AUDITING
#endif

#if defined(MESOSPHERE_BUILD_FOR_AUDITING) || defined(AMS_BUILD_FOR_DEBUGGING)
#define MESOSPHERE_BUILD_FOR_DEBUGGING
#endif

#ifdef  MESOSPHERE_BUILD_FOR_DEBUGGING
#define MESOSPHERE_ENABLE_ASSERTIONS
#define MESOSPHERE_ENABLE_DEBUG_PRINT
#endif

#include <mesosphere/svc/kern_svc_results.hpp>
