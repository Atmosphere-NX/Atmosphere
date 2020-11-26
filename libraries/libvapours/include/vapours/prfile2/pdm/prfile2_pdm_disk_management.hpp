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
#include <vapours/prfile2/prfile2_handle.hpp>
#include <vapours/prfile2/pdm/prfile2_pdm_common.hpp>
#include <vapours/prfile2/pdm/prfile2_pdm_disk.hpp>
#include <vapours/prfile2/pdm/prfile2_pdm_partition.hpp>

namespace ams::prfile2::pdm {

    struct PartitionHolder {
        u32 signature;
        Partition *partition;
    };

    struct DiskHolder {
        u32 signature;
        Disk *disk;
    };

    struct DiskSet {
        u16 num_partitions;
        u16 num_allocated_disks;
        DiskHolder disk_holders[MaxDisks];
        PartitionHolder partition_holders[MaxPartitions];
        Disk disks[MaxDisks];
        Partition partitions[MaxPartitions];
    };

}
