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

/* All architectures must define NumArchitectureDeviceRegions. */
constexpr inline const auto NumArchitectureDeviceRegions = 3;

constexpr inline const auto KMemoryRegionType_Uart                  = KMemoryRegionType_ArchDeviceBase.DeriveSparse(0, NumArchitectureDeviceRegions, 0);
constexpr inline const auto KMemoryRegionType_InterruptCpuInterface = KMemoryRegionType_ArchDeviceBase.DeriveSparse(0, NumArchitectureDeviceRegions, 1).SetAttribute(KMemoryRegionAttr_NoUserMap);
constexpr inline const auto KMemoryRegionType_InterruptDistributor  = KMemoryRegionType_ArchDeviceBase.DeriveSparse(0, NumArchitectureDeviceRegions, 2).SetAttribute(KMemoryRegionAttr_NoUserMap);
static_assert(KMemoryRegionType_Uart                 .GetValue() == (0x1D));
static_assert(KMemoryRegionType_InterruptCpuInterface.GetValue() == (0x2D | KMemoryRegionAttr_NoUserMap));
static_assert(KMemoryRegionType_InterruptDistributor .GetValue() == (0x4D | KMemoryRegionAttr_NoUserMap));
