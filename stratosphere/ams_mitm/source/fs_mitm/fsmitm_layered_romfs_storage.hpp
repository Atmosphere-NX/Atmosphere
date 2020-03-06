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
#include <stratosphere.hpp>
#include "fsmitm_romfs.hpp"

namespace ams::mitm::fs {

    class LayeredRomfsStorage : public std::enable_shared_from_this<LayeredRomfsStorage>, public ams::fs::IStorage {
        private:
            std::vector<romfs::SourceInfo> source_infos;
            std::unique_ptr<ams::fs::IStorage> storage_romfs;
            std::unique_ptr<ams::fs::IStorage> file_romfs;
            os::Event initialize_event;
            ncm::ProgramId program_id;
            bool is_initialized;
            bool started_initialize;
        protected:
            inline s64 GetSize() const {
                const auto &back = this->source_infos.back();
                return back.virtual_offset + back.size;
            }
        public:
            LayeredRomfsStorage(std::unique_ptr<ams::fs::IStorage> s_r, std::unique_ptr<ams::fs::IStorage> f_r, ncm::ProgramId pr_id);
            virtual ~LayeredRomfsStorage();

            void BeginInitialize();
            void InitializeImpl();

            std::shared_ptr<LayeredRomfsStorage> GetShared() {
                return this->shared_from_this();
            }

            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result GetSize(s64 *out_size) override;
            virtual Result Flush() override;
            virtual Result OperateRange(void *dst, size_t dst_size, ams::fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override;

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                /* TODO: Better result code? */
                return ams::fs::ResultUnsupportedOperation();
            }

            virtual Result SetSize(s64 size) override {
                /* TODO: Better result code? */
                return ams::fs::ResultUnsupportedOperation();
            }
    };

}
