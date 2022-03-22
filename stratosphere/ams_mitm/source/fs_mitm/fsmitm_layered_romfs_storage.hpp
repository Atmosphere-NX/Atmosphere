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
#include "fsmitm_romfs.hpp"

namespace ams::mitm::fs {

    class LayeredRomfsStorageImpl {
        private:
            std::vector<romfs::SourceInfo> m_source_infos;
            std::unique_ptr<ams::fs::IStorage> m_storage_romfs;
            std::unique_ptr<ams::fs::IStorage> m_file_romfs;
            os::Event m_initialize_event;
            ncm::ProgramId m_program_id;
            bool m_is_initialized;
            bool m_started_initialize;
        protected:
            inline s64 GetSize() const {
                const auto &back = m_source_infos.back();
                return back.virtual_offset + back.size;
            }
        public:
            LayeredRomfsStorageImpl(std::unique_ptr<ams::fs::IStorage> s_r, std::unique_ptr<ams::fs::IStorage> f_r, ncm::ProgramId pr_id);
            ~LayeredRomfsStorageImpl();

            void BeginInitialize();
            void InitializeImpl();

            constexpr ncm::ProgramId GetProgramId() const { return m_program_id; }

            Result Read(s64 offset, void *buffer, size_t size);
            Result GetSize(s64 *out_size);
            Result Flush();
            Result OperateRange(void *dst, size_t dst_size, ams::fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size);
    };

    std::shared_ptr<ams::fs::IStorage> GetLayeredRomfsStorage(ncm::ProgramId program_id, ::FsStorage &data_storage, bool is_process_romfs);

}
