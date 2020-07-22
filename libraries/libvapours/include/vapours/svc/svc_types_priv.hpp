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

namespace ams::svc {

    namespace lp64 {

        struct CreateProcessParameter {
            char   name[12];
            u32    version;
            u64    program_id;
            u64    code_address;
            s32    code_num_pages;
            u32    flags;
            Handle reslimit;
            s32    system_resource_num_pages;
        };
        static_assert(sizeof(CreateProcessParameter) == 0x30);

    }

    namespace ilp32 {

        struct CreateProcessParameter {
            char   name[12];
            u32    version;
            u64    program_id;
            u64    code_address;
            s32    code_num_pages;
            u32    flags;
            Handle reslimit;
            s32    system_resource_num_pages;
        };
        static_assert(sizeof(CreateProcessParameter) == 0x30);

    }

}
