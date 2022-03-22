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
#include <vapours/svc/svc_types_common.hpp>

namespace ams::svc {

    namespace lp64 {

        struct MemoryInfo {
            u64 base_address;
            u64 size;
            MemoryState state;
            MemoryAttribute attribute;
            MemoryPermission permission;
            u32 ipc_count;
            u32 device_count;
            u32 padding;
        };

        struct LastThreadContext {
            u64 fp;
            u64 sp;
            u64 lr;
            u64 pc;
        };

    }

    namespace ilp32 {

        struct MemoryInfo {
            u64 base_address;
            u64 size;
            MemoryState state;
            MemoryAttribute attribute;
            MemoryPermission permission;
            u32 ipc_count;
            u32 device_count;
            u32 padding;
        };

        struct LastThreadContext {
            u32 fp;
            u32 sp;
            u32 lr;
            u32 pc;
        };

    }

}
