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
#include <vapours/sdmmc/sdmmc_build_config.hpp>


namespace ams::sdmmc {

    enum MmcPartition {
        MmcPartition_UserData       = 0,
        MmcPartition_BootPartition1 = 1,
        MmcPartition_BootPartition2 = 2,
        MmcPartition_Unknown        = 3,
    };

    constexpr inline size_t MmcExtendedCsdSize = 0x200;
    constexpr inline size_t MmcWorkBufferSize  = MmcExtendedCsdSize;

    void SetMmcWorkBuffer(Port port, void *buffer, size_t buffer_size);
    void PutMmcToSleep(Port port);
    void AwakenMmc(Port port);
    Result SelectMmcPartition(Port port, MmcPartition mmc_partition);
    Result EraseMmc(Port port);
    Result GetMmcBootPartitionCapacity(u32 *out_num_sectors, Port port);
    Result GetMmcExtendedCsd(void *out_buffer, size_t buffer_size, Port port);

    Result CheckMmcConnection(SpeedMode *out_speed_mode, BusWidth *out_bus_width, Port port);

}