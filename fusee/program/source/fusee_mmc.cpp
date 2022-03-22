/*
 * Copyright (c) Atmosphère-NX
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
#include <exosphere.hpp>
#include "fusee_mmc.hpp"

namespace ams::nxboot {

    namespace {

        constexpr inline auto MmcPort = sdmmc::Port_Mmc0;

        alignas(0x10) constinit u8 g_mmc_work_buffer[sdmmc::MmcWorkBufferSize];

        constinit inline auto g_mmc_partition = sdmmc::MmcPartition_Unknown;

        Result SelectMmcPartition(sdmmc::MmcPartition partition) {
            /* Change partition, if we need to. */
            if (partition != g_mmc_partition) {
                R_TRY(sdmmc::SelectMmcPartition(MmcPort, partition));

                g_mmc_partition = partition;
            }

            return ResultSuccess();
        }

    }

    Result InitializeMmc() {
        /* Initialize the mmc. */
        sdmmc::Initialize(MmcPort);

        /* Set the mmc work buffer. */
        sdmmc::SetMmcWorkBuffer(MmcPort, g_mmc_work_buffer, sizeof(g_mmc_work_buffer));

        /* Activate the mmc. */
        return sdmmc::Activate(MmcPort);
    }

    Result CheckMmcConnection(sdmmc::SpeedMode *out_sm, sdmmc::BusWidth *out_bw) {
        return sdmmc::CheckMmcConnection(out_sm, out_bw, MmcPort);
    }

    Result GetMmcMemoryCapacity(u32 *out_num_sectors, sdmmc::MmcPartition partition) {
        if (partition == sdmmc::MmcPartition_UserData) {
            return sdmmc::GetDeviceMemoryCapacity(out_num_sectors, MmcPort);
        } else {
            return sdmmc::GetMmcBootPartitionCapacity(out_num_sectors, MmcPort);
        }
    }

    Result ReadMmc(void *dst, size_t size, sdmmc::MmcPartition partition, size_t sector_index, size_t sector_count) {
        R_TRY(SelectMmcPartition(partition));
        return sdmmc::Read(dst, size, MmcPort, sector_index, sector_count);
    }

    Result WriteMmc(sdmmc::MmcPartition partition, size_t sector_index, size_t sector_count, const void *src, size_t size) {
        R_TRY(SelectMmcPartition(partition));
        return sdmmc::Write(MmcPort, sector_index, sector_count, src, size);
    }

}