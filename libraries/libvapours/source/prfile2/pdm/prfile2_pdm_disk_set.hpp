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

namespace ams::prfile2::pdm {

    namespace impl {

        extern DiskSet g_disk_set;

    }

    ALWAYS_INLINE Disk *GetDisk(HandleType handle) {
        if (AMS_LIKELY(IsDiskHandle(handle))) {
            if (const auto id = handle.Get<HandleField::Id>(); AMS_LIKELY(id < MaxDisks)) {
                const auto signature = handle.Get<HandleField::Signature>();
                Disk *disk = std::addressof(impl::g_disk_set.disks[id]);

                for (const auto &holder : impl::g_disk_set.disk_holders) {
                    if (holder.disk == disk && holder.signature == signature) {
                        return disk;
                    }
                }
            }
        }

        return nullptr;
    }

    ALWAYS_INLINE Partition *GetPartition(HandleType handle) {
        if (AMS_LIKELY(IsPartitionHandle(handle))) {
            if (const auto id = handle.Get<HandleField::Id>(); AMS_LIKELY(id < MaxPartitions)) {
                const auto signature = handle.Get<HandleField::Signature>();
                Partition *part = std::addressof(impl::g_disk_set.partitions[id]);

                for (const auto &holder : impl::g_disk_set.partition_holders) {
                    if (holder.partition == part && holder.signature == signature) {
                        return part;
                    }
                }
            }
        }

        return nullptr;
    }

}
