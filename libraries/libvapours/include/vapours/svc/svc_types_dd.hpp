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
#include "svc_types_common.hpp"

#ifdef ATMOSPHERE_BOARD_NINTENDO_SWITCH

    #include "board/nintendo/switch/svc_device_name.hpp"

#else

    #error "Unknown board for svc::DeviceName"

#endif

namespace ams::svc {

    namespace lp64 {

        struct PhysicalMemoryInfo {
            PhysicalAddress physical_address;
            u64 virtual_address;
            u64 size;
        };

    }

    namespace ilp32 {

        struct PhysicalMemoryInfo {
            PhysicalAddress physical_address;
            u32 virtual_address;
            u32 size;
        };

    }

}
