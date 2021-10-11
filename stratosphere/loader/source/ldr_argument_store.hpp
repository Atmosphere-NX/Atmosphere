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
#include <stratosphere.hpp>

namespace ams::ldr {

    class ArgumentStore {
        public:
            static constexpr size_t ArgumentBufferSize = 32_KB;

            struct Entry {
                ncm::ProgramId program_id;
                size_t argument_size;
                u8 argument[ArgumentBufferSize];
            };
        private:
            static constexpr int ArgumentMapCount = 10;
        private:
            Entry m_argument_map[ArgumentMapCount];
        public:
            constexpr ArgumentStore() : m_argument_map{} {
                this->Flush();
            }
        public:
            const Entry *Get(ncm::ProgramId program_id);
            Result Set(ncm::ProgramId program_id, const void *argument, size_t size);

            constexpr Result Flush() {
                for (auto &entry : m_argument_map) {
                    entry.program_id = ncm::InvalidProgramId;
                }
                return ResultSuccess();
            }
        private:
            static int FindIndex(Entry *map, ncm::ProgramId program_id);
    };

}
