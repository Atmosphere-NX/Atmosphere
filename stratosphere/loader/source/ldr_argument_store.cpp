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
#include "ldr_argument_store.hpp"

namespace ams::ldr {

    int ArgumentStore::FindIndex(Entry *map, ncm::ProgramId program_id) {
        for (auto i = 0; i < ArgumentMapCount; ++i) {
            if (map[i].program_id == program_id) {
                return i;
            }
        }

        return -1;
    }

    const ArgumentStore::Entry *ArgumentStore::Get(ncm::ProgramId program_id) {
        /* Find the matching arguments entry. */
        Entry *argument = nullptr;
        if (auto idx = FindIndex(m_argument_map, program_id); idx >= 0) {
            argument = m_argument_map + idx;
        }

        return argument;
    }

    Result ArgumentStore::Set(ncm::ProgramId program_id, const void *argument, size_t size) {
        /* Check that the argument size is within bounds. */
        R_UNLESS(size < ArgumentBufferSize, ldr::ResultArgumentOverflow());

        /* Find either a matching arguments entry, or an empty entry. */
        auto idx = FindIndex(m_argument_map, program_id);
        if (idx < 0) {
            idx = FindIndex(m_argument_map, ncm::InvalidProgramId);
        }
        R_UNLESS(idx >= 0, ldr::ResultArgumentCountOverflow());

        /* Set the arguments in the entry. */
        auto &entry = m_argument_map[idx];

        entry.program_id    = program_id;
        entry.argument_size = size;
        std::memcpy(entry.argument, argument, entry.argument_size);

        return ResultSuccess();
    }

}
