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
#include <stratosphere.hpp>
#include "impl/fssrv_program_info.hpp"

namespace ams::fssrv {

    Result ProgramRegistryServiceImpl::RegisterProgramInfo(u64 process_id, u64 program_id, u8 storage_id, const void *data, s64 data_size, const void *desc, s64 desc_size) {
        AMS_ABORT("TODO");
        AMS_UNUSED(process_id, program_id, storage_id, data, data_size, desc, desc_size);
    }

    Result ProgramRegistryServiceImpl::UnregisterProgramInfo(u64 process_id) {
        AMS_ABORT("TODO");
        AMS_UNUSED(process_id);
    }

}
