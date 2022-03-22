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
#include <vapours.hpp>
#pragma once

namespace ams::nxboot {

    Result InitializeMmc();
    Result CheckMmcConnection(sdmmc::SpeedMode *out_sm, sdmmc::BusWidth *out_bw);
    Result GetMmcMemoryCapacity(u32 *out_num_sectors, sdmmc::MmcPartition partition);

    Result ReadMmc(void *dst, size_t size, sdmmc::MmcPartition partition, size_t sector_index, size_t sector_count);
    Result WriteMmc(sdmmc::MmcPartition partition, size_t sector_index, size_t sector_count, const void *src, size_t size);

}