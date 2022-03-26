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
#include "fssrv_program_registry_manager.hpp"

namespace ams::fssrv::impl {

    Result ProgramRegistryManager::RegisterProgram(u64 process_id, u64 program_id, u8 storage_id, const void *data, s64 data_size, const void *desc, s64 desc_size) {
        /* Allocate a new node. */
        std::unique_ptr<ProgramInfoNode> new_node(new ProgramInfoNode());
        R_UNLESS(new_node != nullptr, fs::ResultAllocationMemoryFailedInProgramRegistryManagerA());

        /* Create a new program info. */
        {
            /* Allocate the new info. */
            auto new_info = fssystem::AllocateShared<ProgramInfo>(process_id, program_id, storage_id, data, data_size, desc, desc_size);
            R_UNLESS(new_info != nullptr, fs::ResultAllocationMemoryFailedInProgramRegistryManagerA());

            /* Set the info in the node. */
            new_node->program_info = std::move(new_info);
        }

        /* Acquire exclusive access to the registry. */
        std::scoped_lock lk(m_mutex);

        /* Check that the process isn't already in the registry. */
        for (const auto &node : m_program_info_list) {
            R_UNLESS(!node.program_info->Contains(process_id), fs::ResultInvalidArgument());
        }

        /* Add the node to the registry. */
        m_program_info_list.push_back(*new_node.release());

        R_SUCCEED();
    }

    Result ProgramRegistryManager::UnregisterProgram(u64 process_id) {
        /* Acquire exclusive access to the registry. */
        std::scoped_lock lk(m_mutex);

        /* Try to find and remove the process's node. */
        for (auto &node : m_program_info_list) {
            if (node.program_info->Contains(process_id)) {
                m_program_info_list.erase(m_program_info_list.iterator_to(node));
                delete std::addressof(node);
                R_SUCCEED();
            }
        }

        /* We couldn't find/unregister the process's node. */
        R_THROW(fs::ResultInvalidArgument());
    }

    Result ProgramRegistryManager::GetProgramInfo(std::shared_ptr<ProgramInfo> *out, u64 process_id) {
        /* Acquire exclusive access to the registry. */
        std::scoped_lock lk(m_mutex);

        /* Check if we're getting permissions for an initial program. */
        if (IsInitialProgram(process_id)) {
            *out = ProgramInfo::GetProgramInfoForInitialProcess();
            R_SUCCEED();
        }

        /* Find a matching node. */
        for (const auto &node : m_program_info_list) {
            if (node.program_info->Contains(process_id)) {
                *out = node.program_info;
                R_SUCCEED();
            }
        }

        /* We didn't find the program info. */
        R_THROW(fs::ResultProgramInfoNotFound());
    }

    Result ProgramRegistryManager::GetProgramInfoByProgramId(std::shared_ptr<ProgramInfo> *out, u64 program_id) {
        /* Acquire exclusive access to the registry. */
        std::scoped_lock lk(m_mutex);

        /* Find a matching node. */
        for (const auto &node : m_program_info_list) {
            if (node.program_info->GetProgramIdValue() == program_id) {
                *out = node.program_info;
                R_SUCCEED();
            }
        }

        /* We didn't find the program info. */
        R_THROW(fs::ResultProgramInfoNotFound());
    }

}
