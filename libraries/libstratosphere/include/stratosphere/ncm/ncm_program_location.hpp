/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
#include <stratosphere/ncm/ncm_program_id.hpp>
#include <stratosphere/ncm/ncm_storage_id.hpp>

namespace ams::ncm {

    struct ProgramLocation {
        ProgramId program_id;
        u8 storage_id;

        static constexpr ProgramLocation Make(ProgramId program_id, StorageId storage_id) {
            return { .program_id = program_id, .storage_id = static_cast<u8>(storage_id), };
        }
    };
    static_assert(sizeof(ProgramLocation) == 0x10 && util::is_pod<ProgramLocation>::value);
    static_assert(sizeof(ProgramLocation) == sizeof(::NcmProgramLocation) && alignof(ProgramLocation) == alignof(::NcmProgramLocation), "ProgramLocation Libnx Compatibility");


}
