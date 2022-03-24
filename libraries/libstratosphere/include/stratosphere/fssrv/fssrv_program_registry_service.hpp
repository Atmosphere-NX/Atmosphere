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
#include <vapours.hpp>
#include <stratosphere/fssystem/fssystem_pimpl.hpp>

namespace ams::fssrv {

    namespace impl {

        class ProgramInfo;
        class ProgramRegistryManager;
        class ProgramIndexMapInfoManager;

    }

    /* ACCURATE_TO_VERSION: Unknown */
    class ProgramRegistryServiceImpl {
        public:
            struct Configuration {
                /* ... */
            };
        private:
            Configuration m_config;
            fssystem::Pimpl<impl::ProgramRegistryManager, 0x40> m_registry_manager;
            fssystem::Pimpl<impl::ProgramIndexMapInfoManager, 0x50> m_index_map_info_manager;
        public:
            ProgramRegistryServiceImpl(const Configuration &cfg) : m_config(cfg) { /* ... */ }

            Result RegisterProgramInfo(u64 process_id, u64 program_id, u8 storage_id, const void *data, s64 data_size, const void *desc, s64 desc_size);
            Result UnregisterProgramInfo(u64 process_id);

            Result ResetProgramIndexMapInfo(const fs::ProgramIndexMapInfo *infos, int count);

            Result GetProgramInfo(std::shared_ptr<impl::ProgramInfo> *out, u64 process_id);
            Result GetProgramInfoByProgramId(std::shared_ptr<impl::ProgramInfo> *out, u64 program_id);

            size_t GetProgramIndexMapInfoCount();
            util::optional<fs::ProgramIndexMapInfo> GetProgramIndexMapInfo(const ncm::ProgramId &program_id);

            ncm::ProgramId GetProgramIdByIndex(const ncm::ProgramId &program_id, u8 index);
    };

}
