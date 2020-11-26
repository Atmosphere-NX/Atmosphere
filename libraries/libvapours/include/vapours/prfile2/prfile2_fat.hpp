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
#include <vapours/prfile2/prfile2_common.hpp>

namespace ams::prfile2 {

    struct FatHint {
        u32 chain_index;
        u32 cluster;
    };

    using LastAccess = FatHint;

    struct LastCluster {
        u32 num_last_cluster;
        u32 max_chain_index;
    };

    struct ClusterLinkForVolume {
        u16 flag;
        u16 interval;
        u32 *buffer;
        u32 link_max;
    };

    using ClusterBuf = u32;

    struct ClusterLink {
        u64 position;
        ClusterBuf *buffer;
        u16 interval;
        u16 interval_offset;
        u32 save_index;
        u32 max_count;
    };

    struct Volume;

    struct FatFileDescriptor {
        u32 start_cluster;
        u32 *p_start_cluster;
        LastCluster last_cluster;
        LastAccess last_access;
        ClusterLink cluster_link;
        FatHint *hint;
        Volume *volume;
        /* ... */
    };

}
