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
constexpr inline const auto NumBoardDeviceRegions = 5;
                                                                                                /* UNUSED: .Derive(NumBoardDeviceRegions, 0); */
constexpr inline const auto KMemoryRegionType_MemoryController          = KMemoryRegionType_BoardDeviceBase.Derive(NumBoardDeviceRegions, 1).SetAttribute(KMemoryRegionAttr_NoUserMap);
constexpr inline const auto KMemoryRegionType_MemoryController1         = KMemoryRegionType_BoardDeviceBase.Derive(NumBoardDeviceRegions, 2).SetAttribute(KMemoryRegionAttr_NoUserMap);
constexpr inline const auto KMemoryRegionType_MemoryController0         = KMemoryRegionType_BoardDeviceBase.Derive(NumBoardDeviceRegions, 3).SetAttribute(KMemoryRegionAttr_NoUserMap);
constexpr inline const auto KMemoryRegionType_PowerManagementController = KMemoryRegionType_BoardDeviceBase.Derive(NumBoardDeviceRegions, 4).DeriveTransition();
static_assert(KMemoryRegionType_MemoryController         .GetValue() == (0x55  | KMemoryRegionAttr_NoUserMap));
static_assert(KMemoryRegionType_MemoryController1        .GetValue() == (0x65  | KMemoryRegionAttr_NoUserMap));
static_assert(KMemoryRegionType_MemoryController0        .GetValue() == (0x95  | KMemoryRegionAttr_NoUserMap));
static_assert(KMemoryRegionType_PowerManagementController.GetValue() == (0x1A5));
