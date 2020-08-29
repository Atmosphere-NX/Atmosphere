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

/* All architectures must define NumBoardDeviceRegions. */
constexpr inline const auto NumBoardDeviceRegions = 6;
                                                                                                /* UNUSED: .Derive(NumBoardDeviceRegions, 0); */
constexpr inline const auto KMemoryRegionType_MemoryController          = KMemoryRegionType_BoardDeviceBase.Derive(NumBoardDeviceRegions, 1).SetAttribute(KMemoryRegionAttr_NoUserMap);
constexpr inline const auto KMemoryRegionType_MemoryController1         = KMemoryRegionType_BoardDeviceBase.Derive(NumBoardDeviceRegions, 2).SetAttribute(KMemoryRegionAttr_NoUserMap);
constexpr inline const auto KMemoryRegionType_MemoryController0         = KMemoryRegionType_BoardDeviceBase.Derive(NumBoardDeviceRegions, 3).SetAttribute(KMemoryRegionAttr_NoUserMap);
constexpr inline const auto KMemoryRegionType_PowerManagementController = KMemoryRegionType_BoardDeviceBase.Derive(NumBoardDeviceRegions, 4).DeriveTransition();
constexpr inline const auto KMemoryRegionType_LegacyLpsDevices          = KMemoryRegionType_BoardDeviceBase.Derive(NumBoardDeviceRegions, 5);
static_assert(KMemoryRegionType_MemoryController         .GetValue() == (0x55  | KMemoryRegionAttr_NoUserMap));
static_assert(KMemoryRegionType_MemoryController1        .GetValue() == (0x65  | KMemoryRegionAttr_NoUserMap));
static_assert(KMemoryRegionType_MemoryController0        .GetValue() == (0x95  | KMemoryRegionAttr_NoUserMap));
static_assert(KMemoryRegionType_PowerManagementController.GetValue() == (0x1A5));

static_assert(KMemoryRegionType_LegacyLpsDevices.GetValue() == 0xC5);

constexpr inline const auto NumLegacyLpsDevices = 7;
constexpr inline const auto KMemoryRegionType_LegacyLpsExceptionVectors = KMemoryRegionType_LegacyLpsDevices.Derive(NumLegacyLpsDevices, 0);
constexpr inline const auto KMemoryRegionType_LegacyLpsIram             = KMemoryRegionType_LegacyLpsDevices.Derive(NumLegacyLpsDevices, 1);
constexpr inline const auto KMemoryRegionType_LegacyLpsFlowController   = KMemoryRegionType_LegacyLpsDevices.Derive(NumLegacyLpsDevices, 2);
constexpr inline const auto KMemoryRegionType_LegacyLpsPrimaryICtlr     = KMemoryRegionType_LegacyLpsDevices.Derive(NumLegacyLpsDevices, 3);
constexpr inline const auto KMemoryRegionType_LegacyLpsSemaphore        = KMemoryRegionType_LegacyLpsDevices.Derive(NumLegacyLpsDevices, 4);
constexpr inline const auto KMemoryRegionType_LegacyLpsAtomics          = KMemoryRegionType_LegacyLpsDevices.Derive(NumLegacyLpsDevices, 5);
constexpr inline const auto KMemoryRegionType_LegacyLpsClkRst           = KMemoryRegionType_LegacyLpsDevices.Derive(NumLegacyLpsDevices, 6);
static_assert(KMemoryRegionType_LegacyLpsExceptionVectors.GetValue() == 0x3C5);
static_assert(KMemoryRegionType_LegacyLpsIram            .GetValue() == 0x5C5);
static_assert(KMemoryRegionType_LegacyLpsFlowController  .GetValue() == 0x6C5);
static_assert(KMemoryRegionType_LegacyLpsPrimaryICtlr    .GetValue() == 0x9C5);
static_assert(KMemoryRegionType_LegacyLpsSemaphore       .GetValue() == 0xAC5);
static_assert(KMemoryRegionType_LegacyLpsAtomics         .GetValue() == 0xCC5);
static_assert(KMemoryRegionType_LegacyLpsClkRst          .GetValue() == 0x11C5);
