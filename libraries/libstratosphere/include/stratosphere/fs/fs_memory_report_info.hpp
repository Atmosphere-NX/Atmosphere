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
#pragma once
#include <stratosphere/fs/fs_common.hpp>

namespace ams::fs {

    struct MemoryReportInfo {
        u64 pooled_buffer_peak_free_size;
        u64 pooled_buffer_retried_count;
        u64 pooled_buffer_reduce_allocation_count;
        u64 buffer_manager_peak_free_size;
        u64 buffer_manager_retried_count;
        u64 exp_heap_peak_free_size;
        u64 buffer_pool_peak_free_size;
        u64 patrol_read_allocate_buffer_success_count;
        u64 patrol_read_allocate_buffer_failure_count;
        u64 buffer_manager_peak_total_allocatable_size;
        u64 buffer_pool_max_allocate_size;
        u64 pooled_buffer_failed_ideal_allocation_count_on_async_access;

        u8 reserved[0x20];
    };
    static_assert(sizeof(MemoryReportInfo) == 0x80);
    static_assert(util::is_pod<MemoryReportInfo>::value);

    Result GetAndClearMemoryReportInfo(MemoryReportInfo *out);

}
