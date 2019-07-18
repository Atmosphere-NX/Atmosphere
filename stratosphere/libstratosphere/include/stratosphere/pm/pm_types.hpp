/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>

namespace sts::pm {

    enum class BootMode {
        Normal      = 0,
        Maintenance = 1,
        SafeMode    = 2,
    };

    enum ResourceLimitGroup {
        ResourceLimitGroup_System      = 0,
        ResourceLimitGroup_Application = 1,
        ResourceLimitGroup_Applet      = 2,
        ResourceLimitGroup_Count,
    };

    using LimitableResource = ::LimitableResource;

    struct ProcessEventInfo {
        u32 event;
        u64 process_id;
    };
    static_assert(sizeof(ProcessEventInfo) == 0x10 && std::is_pod<ProcessEventInfo>::value, "ProcessEventInfo definition!");

}
