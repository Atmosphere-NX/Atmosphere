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
#include "impl/fssrv_program_registry_manager.hpp"

namespace ams::fssrv {

    Result ProgramRegistryServiceImpl::RegisterProgramInfo(u64 process_id, u64 program_id, u8 storage_id, const void *data, s64 data_size, const void *desc, s64 desc_size) {
        R_RETURN(m_registry_manager->RegisterProgram(process_id, program_id, storage_id, data, data_size, desc, desc_size));
    }

    Result ProgramRegistryServiceImpl::UnregisterProgramInfo(u64 process_id) {
        R_RETURN(m_registry_manager->UnregisterProgram(process_id));
    }

    Result ProgramRegistryServiceImpl::ResetProgramIndexMapInfo(const fs::ProgramIndexMapInfo *infos, int count) {
        R_RETURN(m_index_map_info_manager->Reset(infos, count));
    }

    Result ProgramRegistryServiceImpl::GetProgramInfo(std::shared_ptr<impl::ProgramInfo> *out, u64 process_id) {
        R_RETURN(m_registry_manager->GetProgramInfo(out, process_id));
    }

    Result ProgramRegistryServiceImpl::GetProgramInfoByProgramId(std::shared_ptr<impl::ProgramInfo> *out, u64 program_id) {
        R_RETURN(m_registry_manager->GetProgramInfoByProgramId(out, program_id));
    }

    size_t ProgramRegistryServiceImpl::GetProgramIndexMapInfoCount() {
        return m_index_map_info_manager->GetProgramCount();
    }

    util::optional<fs::ProgramIndexMapInfo> ProgramRegistryServiceImpl::GetProgramIndexMapInfo(const ncm::ProgramId &program_id) {
        return m_index_map_info_manager->Get(program_id);
    }

    ncm::ProgramId ProgramRegistryServiceImpl::GetProgramIdByIndex(const ncm::ProgramId &program_id, u8 index) {
        return m_index_map_info_manager->GetProgramId(program_id, index);
    }

}
