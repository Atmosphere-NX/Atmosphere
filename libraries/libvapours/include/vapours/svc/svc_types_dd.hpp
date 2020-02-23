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
#include <vapours/svc/svc_types_common.hpp>
#include <vapours/svc/svc_select_hardware_constants.hpp>
#include <vapours/svc/svc_select_device_name.hpp>

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
